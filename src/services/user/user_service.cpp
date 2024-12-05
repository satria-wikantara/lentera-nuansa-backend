//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/pch/pch.h"

#include "nuansa/models/user.h"
#include "nuansa/services/user/user_service.h"
#include "nuansa/database/db_connection_pool.h"
#include "nuansa/config/config.h"
#include "nuansa/utils/crypto/crypto_util.h"
#include "nuansa/utils/validation/validation.h"

namespace nuansa::services::user {

    UserService &UserService::GetInstance() {
        static UserService instance;
        return instance;
    }

    void UserService::Initialize() {
        try {
            if (!nuansa::database::ConnectionPool::GetInstance().IsInitialized()) {
                BOOST_LOG_TRIVIAL(warning) << "Connection pool not initialized during UserService initialization";
                return;
            }

            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection(
                std::chrono::milliseconds(1000) // Short timeout for initialization
            );

            if (conn) {
                fallbackConnection_ = std::move(conn);
                BOOST_LOG_TRIVIAL(info) << "UserService initialized with fallback connection";
            }
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(warning) << "Failed to acquire fallback connection: " << e.what();
            // Don't throw, continue with null fallback connection
        }

        // Reset circuit breaker state
        circuitBreaker_.Reset();
    }

    pqxx::connection *UserService::GetConnection() const {
        try {
            return nuansa::database::ConnectionPool::GetInstance().AcquireConnection().get();
        } catch (const std::exception &) {
            if (fallbackConnection_ &&fallbackConnection_->is_open()
            ) {
                return const_cast<pqxx::connection *>(&(*fallbackConnection_));
            }
            throw;
        }
    }

    std::optional<nuansa::models::User> UserService::GetUserByUsername(const std::string &username) {
        try {
            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection();
            nuansa::database::ConnectionGuard guard(conn);

            return guard.ExecuteWithRetry([&](pqxx::connection &conn) {
                pqxx::work txn{conn};

                auto result = txn.exec_params(
                    "SELECT username, email, password_hash, salt FROM users WHERE username = $1",
                    username);

                if (result.empty()) {
                    return std::optional<nuansa::models::User>();
                }

                return std::optional<nuansa::models::User>(nuansa::models::User(
                    result[0]["username"].as<std::string>(),
                    result[0]["email"].as<std::string>(),
                    result[0]["password_hash"].as<std::string>(),
                    result[0]["salt"].as<std::string>()
                ));
            });
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error retrieving user by username: " << e.what();
            return std::nullopt;
        }
    }

    std::optional<nuansa::models::User> UserService::GetUserByEmail(const std::string &email) {
        try {
            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection();
            nuansa::database::ConnectionGuard guard(conn);
            pqxx::work txn{*conn};

            auto result = txn.exec_params(
                "SELECT username, email, password_hash, salt FROM users WHERE email = $1",
                email);

            if (result.empty()) {
                return std::nullopt;
            }

            return nuansa::models::User(
                result[0]["username"].as<std::string>(),
                result[0]["email"].as<std::string>(),
                result[0]["password_hash"].as<std::string>(),
                result[0]["salt"].as<std::string>()
            );
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error retrieving user by email: " << e.what();
            return std::nullopt;
        }
    }

    bool UserService::IsEmailTaken(const std::string &email) const {
        try {
            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection();
            nuansa::database::ConnectionGuard guard(conn);
            pqxx::work txn{*conn};

            auto result = txn.exec_params(
                "SELECT 1 FROM users WHERE email = $1",
                email);

            return !result.empty();
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error checking email: " << e.what();
            return false;
        }
    }

    bool UserService::IsUsernameTaken(const std::string &username) const {
        try {
            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection();
            nuansa::database::ConnectionGuard guard(conn);
            pqxx::work txn{*conn};

            auto result = txn.exec_params(
                "SELECT 1 FROM users WHERE username = $1",
                username);

            return !result.empty();
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error checking username: " << e.what();
            return false;
        }
    }

    namespace {
        constexpr size_t SALT_LENGTH = 16;
        constexpr size_t HASH_LENGTH = 32;
        constexpr int ITERATIONS = 10000;

        std::string bytesToHex(const unsigned char *data, size_t len) {
            std::stringstream ss;
            ss << std::hex << std::setfill('0');
            for (size_t i = 0; i < len; i++) {
                ss << std::setw(2) << static_cast<int>(data[i]);
            }
            return ss.str();
        }

        std::vector<unsigned char> hexToBytes(const std::string &hex) {
            std::vector<unsigned char> bytes;
            for (size_t i = 0; i < hex.length(); i += 2) {
                std::string byteString = hex.substr(i, 2);
                unsigned char byte = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
                bytes.push_back(byte);
            }
            return bytes;
        }
    }

    std::string UserService::HashPassword(const std::string &password) {
        // Generate random salt
        std::vector<unsigned char> salt(SALT_LENGTH);
        std::random_device rd;
        std::generate(salt.begin(), salt.end(), std::ref(rd));

        // Generate hash
        std::vector<unsigned char> hash(HASH_LENGTH);
        PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                          salt.data(), salt.size(),
                          ITERATIONS,
                          EVP_sha256(),
                          HASH_LENGTH, hash.data());

        // Format: iterations$salt$hash
        return std::to_string(ITERATIONS) + "$" +
               bytesToHex(salt.data(), salt.size()) + "$" +
               bytesToHex(hash.data(), hash.size());
    }


    bool UserService::CreateUser(const nuansa::models::User &user) {
        if (circuitBreaker_.IsOpen()) {
            BOOST_LOG_TRIVIAL(warning) << "Circuit breaker is open, registration rejected";
            return false;
        }

        try {
            // Create user with the salt and hashed password
            nuansa::models::User userToCreate(
                user.GetUsername(),
                user.GetEmail(),
                user.GetPasswordHash(),
                user.GetSalt()
            );


            if (!nuansa::database::ConnectionPool::GetInstance().IsInitialized()) {
                BOOST_LOG_TRIVIAL(error) << "Connection pool not initialized";
                return false;
            }

            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection(
                std::chrono::milliseconds(1000)
            );

            if (!conn) {
                BOOST_LOG_TRIVIAL(error) << "Failed to acquire connection for user creation";
                return false;
            }

            nuansa::database::ConnectionGuard guard(std::move(conn));
            guard.ExecuteWithRetry([&](pqxx::connection &conn) {
                pqxx::work txn(conn);
                txn.exec_params(
                    "INSERT INTO users (username, email, password_hash, salt) VALUES ($1, $2, $3, $4)",
                    user.GetUsername(), user.GetEmail(), user.GetPasswordHash(), user.GetSalt());
                txn.commit();
                return true;
            });

            circuitBreaker_.RecordSuccess();
            return true;
        } catch (const std::exception &e) {
            circuitBreaker_.RecordFailure();
            BOOST_LOG_TRIVIAL(error) << "Database error during user creation: " << e.what();
            return false;
        }
    }


    bool UserService::AuthenticateUser(const std::string &username, const std::string &password) {
        try {
            auto user = GetUserByUsername(username);
            if (!user) {
                return false;
            }

            // Hash the provided password with the stored salt
            std::string hashedPassword = nuansa::utils::crypto::HashPassword(password, user->GetSalt());

            // Compare the hashed password with the stored hash
            return hashedPassword == user->GetPasswordHash();
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error authenticating user: " << e.what();
            return false;
        }
    }

    bool UserService::UpdateUserEmail(const std::string &username, const std::string &newEmail) {
        try {
            if (!nuansa::utils::validation::ValidateEmail(newEmail)) {
                BOOST_LOG_TRIVIAL(error) << "Invalid email format";
                return false;
            }

            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection();
            nuansa::database::ConnectionGuard guard(conn);

            return guard.ExecuteWithRetry([&](pqxx::connection &conn) {
                pqxx::work txn{conn};

                auto result = txn.exec_params(
                    "UPDATE users SET email = $1 WHERE username = $2",
                    newEmail, username);

                if (result.affected_rows() == 0) {
                    return false;
                }

                txn.commit();
                BOOST_LOG_TRIVIAL(info) << "Email updated for user: " << username;
                return true;
            });
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error updating user email: " << e.what();
            return false;
        }
    }

    bool UserService::UpdateUserPassword(const std::string &username, const std::string &newPassword) {
        try {
            // Validate new password
            if (!nuansa::utils::validation::ValidatePassword(newPassword)) {
                BOOST_LOG_TRIVIAL(warning) << "Invalid password format";
                return false;
            }

            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection();
            nuansa::database::ConnectionGuard guard(conn);

            // Generate new salt for the new password
            std::string newSalt = nuansa::utils::crypto::GenerateRandomSalt();

            // Hash the new password with the new salt
            std::string hashedPassword = nuansa::utils::crypto::HashPassword(newPassword, newSalt);

            return guard.ExecuteWithRetry([&](pqxx::connection &conn) {
                pqxx::work txn{conn};

                auto result = txn.exec_params(
                    "UPDATE users SET password_hash = $1, salt = $2 WHERE username = $3",
                    hashedPassword, newSalt, username);

                if (result.affected_rows() == 0) {
                    return false;
                }

                txn.commit();
                BOOST_LOG_TRIVIAL(info) << "Password updated for user: " << username;
                return true;
            });
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error updating user password: " << e.what();
            return false;
        }
    }

    bool UserService::DeleteUser(const std::string &username) {
        try {
            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection();
            nuansa::database::ConnectionGuard guard(conn);

            return guard.ExecuteWithRetry([&](pqxx::connection &conn) {
                pqxx::work txn{conn};

                auto result = txn.exec_params(
                    "DELETE FROM users WHERE username = $1",
                    username);

                if (result.affected_rows() == 0) {
                    return false;
                }

                txn.commit();
                BOOST_LOG_TRIVIAL(info) << "User deleted: " << username;
                return true;
            });
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error deleting user: " << e.what();
            return false;
        }
    }
} // namespace nuansa::services::user