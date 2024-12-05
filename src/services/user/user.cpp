//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/pch/pch.h"

#include "nuansa/models/user.h"

namespace nuansa::models {
    User::User(const std::string &username,
               const std::string &email,
               const std::string &passwordHash,
               const std::string &salt)
        : username(username)
          , email(email)
          , passwordHash(passwordHash)
          , salt(salt) {
    }

    std::string User::GenerateSalt(size_t length) {
        std::vector<unsigned char> salt(length);
        if (RAND_bytes(salt.data(), length) != 1) {
            throw std::runtime_error("Failed to generate salt");
        }

        std::stringstream ss;
        for (const auto &byte: salt) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return ss.str();
    }

    std::string User::HashPassword(const std::string &password, const std::string &salt) {
        std::string salted = salt + password;
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hashLen;

        // Create new EVP_MD_CTX
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
            EVP_MD_CTX_new(), EVP_MD_CTX_free);
        if (!ctx) {
            throw std::runtime_error("Failed to create hash context");
        }

        // Initialize hashing
        if (!EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr)) {
            throw std::runtime_error("Failed to initialize hash");
        }

        // Add data
        if (!EVP_DigestUpdate(ctx.get(), salted.c_str(), salted.length())) {
            throw std::runtime_error("Failed to update hash");
        }

        // Finalize
        if (!EVP_DigestFinal_ex(ctx.get(), hash, &hashLen)) {
            throw std::runtime_error("Failed to finalize hash");
        }

        // Convert to hex string
        std::stringstream ss;
        for (unsigned int i = 0; i < hashLen; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    bool User::VerifyPassword(const std::string &password, const std::string &salt,
                              const std::string &hashedPassword) {
        std::string newHashedPassword = HashPassword(password, salt);
        return hashedPassword == newHashedPassword;
    }

    pqxx::connection User::CreateConnection() {
        try {
            // TODO: Load these from configuration
            return pqxx::connection(
                "postgresql://panca:panca@localhost:5432/nuansa_test"
            );
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Database connection error: " << e.what();
            throw;
        }
    }

    bool User::Authenticate(const std::string &username, const std::string &password) {
        auto user = FindByUsername(username);
        if (!user) {
            return false;
        }

        return VerifyPassword(password, user->salt, user->passwordHash);
    }

    std::optional<User> User::FindByUsername(const std::string &username) {
        try {
            auto conn = CreateConnection();
            pqxx::work txn{conn};

            auto result = txn.exec_params(
                "SELECT id, username, email, password_hash FROM users WHERE username = $1",
                username
            );

            if (result.size() > 0) {
                User user(
                    result[0]["username"].as<std::string>(),
                    result[0]["email"].as<std::string>(),
                    result[0]["password_hash"].as<std::string>(),
                    result[0]["salt"].as<std::string>()
                );
                return user;
            }
            return std::nullopt;
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Failed to find user by username: " << e.what();
            return std::nullopt;
        }
    }
} // namespace App::Models
