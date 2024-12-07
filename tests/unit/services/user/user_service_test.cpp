//
// Created by I Gede Panca Sutresna on 01/12/24.
//
#include "nuansa/utils/pch.h"

#include <gtest/gtest.h>
#include "nuansa/services/user/user_service.h"
#include "nuansa/database/db_connection_pool.h"
#include "nuansa/database/db_connection_guard.h"
#include "nuansa/config/config.h"
#include "nuansa/utils/exception/database_exception.h"
#include "nuansa/utils/pattern/circuit_breaker.h"
#include "nuansa/utils/validation.h"

class UserServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        try {
            BOOST_LOG_TRIVIAL(info) << "Setting up UserServiceTest";

            // Configure test database
            const nuansa::config::DatabaseConfig testConfig{
                .host = "localhost",
                .port = 5432,
                .database_name = "nuansa_test",
                .username = "panca",
                .password = "panca",
                .pool_size = 10
            };

            BOOST_LOG_TRIVIAL(info) << "Setting up server config";
            const nuansa::config::ServerConfig testServerConfig{
                .port = 8080,
                .host = "localhost",
                .logPath = "logs/test.log",
                .logLevel = "debug"
            };

            BOOST_LOG_TRIVIAL(info) << "Setting up config";
            auto &config = nuansa::config::Config::GetInstance();
            config.SetDatabaseConfig(testConfig);
            config.SetServerConfig(testServerConfig);

            // Initialize database connection pool
            const auto connectionString =
                    "postgresql://" + testConfig.username + ":" + testConfig.password +
                    "@" + testConfig.host + ":" + std::to_string(testConfig.port) +
                    "/" + testConfig.database_name;

            BOOST_LOG_TRIVIAL(info) << "Initializing database connection pool";
            auto &connectionPool = nuansa::database::ConnectionPool::GetInstance();
            connectionPool.Shutdown();
            connectionPool.Initialize(connectionString, testConfig.pool_size);

            // Initialize UserService
            auto &userService = nuansa::services::user::UserService::GetInstance();
            userService.Initialize();

            // Clean existing test data
            CleanupTestData();
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Test setup failed: " << e.what();
            throw;
        }
    }

    void TearDown() override {
        try {
            CleanupTestData();
            nuansa::database::ConnectionPool::GetInstance().Shutdown();
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(warning) << "TearDown cleanup failed: " << e.what();
        }
    }

    static void CleanupTestData() {
        try {
            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection(
                std::chrono::milliseconds(1000)
            );

            nuansa::database::ConnectionGuard guard(std::move(conn));

            guard.ExecuteWithRetry([](pqxx::connection &db_conn) {
                pqxx::work txn{db_conn};
                txn.exec("DELETE FROM users");
                txn.commit();
                return true;
            });
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Failed to cleanup test data: " << e.what();
            throw;
        }
    }

    static bool CreateTestUser(const std::string &username = "testuser",
                               const std::string &email = "test@example.com",
                               const std::string &password = "Password123#*") {
        const std::string salt = GenerateRandomSalt();

        // Verify circuit breaker state
        if (auto &circuitBreaker = nuansa::utils::pattern::CircuitBreaker::GetInstance(); circuitBreaker.IsOpen()) {
            BOOST_LOG_TRIVIAL(warning) << "Circuit breaker is OPEN before creating test user";
            circuitBreaker.Reset(); // Force reset if open
        }

        return nuansa::services::user::UserService::GetInstance().CreateUser(
            nuansa::models::User(username, email, password, salt));
    }

    static std::optional<nuansa::models::User> GetTestUser(const std::string &username = "testuser") {
        return nuansa::services::user::UserService::GetInstance()
                .GetUserByUsername(username);
    }

    static bool InitializeConfig(const std::string &configPath) {
        try {
            // Initialize configurations
            auto &config = nuansa::config::GetConfig();
            config.Initialize(configPath);
        } catch (const std::exception &e) {
            throw std::runtime_error("Failed to load config file: " + std::string(e.what()));
        }

        return true;
    }

    static bool InitializeDatabase() {
        const auto &config = nuansa::config::GetConfig();

        try {
            nuansa::database::ConnectionPool::GetInstance().Initialize(
                config.GetDatabaseConfig().connection_string,
                config.GetDatabaseConfig().pool_size
            );
        } catch (const nuansa::utils::exception::DatabaseCreateConnectionException &e) {
            std::cerr << "Database connection pool initialization error: " << e.what() << std::endl;
            return false;
        }

        return true;
    }

    static bool InitializeLogging() {
        const auto &config = nuansa::config::GetConfig();
        std::filesystem::create_directories(std::filesystem::path(config.GetServerConfig().logPath).parent_path());

        boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

        // Only show debug messages when g_runDebug is true
        boost::log::core::get()->set_filter(
            boost::log::trivial::severity >= (config.GetServerConfig().logLevel == "debug"
                                                  ? boost::log::trivial::debug
                                                  : boost::log::trivial::info)
        );

        // Define logging format
        boost::log::add_console_log(
            std::cout,
            boost::log::keywords::format = "[%TimeStamp%] [%Severity%] %Message%"
        );

        // Add common attributes
        boost::log::add_common_attributes();

        return true;
    }


    // Add helper function to generate salt
    static std::string GenerateRandomSalt(const size_t length = 32) {
        static constexpr char charset[] = "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
        std::string salt;
        salt.reserve(length);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

        for (size_t i = 0; i < length; ++i) {
            salt += charset[dis(gen)];
        }
        return salt;
    }
};

// Test successful user registration
TEST_F(UserServiceTest, RegisterUserSuccess) {
    EXPECT_TRUE(CreateTestUser());
}

// Test duplicate username registration
TEST_F(UserServiceTest, RegisterUserDuplicateUsername) {
    EXPECT_TRUE(CreateTestUser());
    EXPECT_FALSE(CreateTestUser("testuser", "other@example.com", "Password123#*"));
}

// Test duplicate email registration
TEST_F(UserServiceTest, RegisterUserDuplicateEmail) {
    EXPECT_TRUE(CreateTestUser("user1", "test@example.com", "Password123#*"));
    EXPECT_FALSE(CreateTestUser("user2", "test@example.com", "Password123#*"));
}

// Test database connection failure
// TEST_F(UserServiceTest, RegisterUserDatabaseError) {
//     auto& connectionPool = nuansa::database::ConnectionPool::GetInstance();

//     // First, properly shutdown the existing pool
//     connectionPool.Shutdown();

//     // Configure invalid database settings
//     nuansa::config::DatabaseConfig invalidConfig{
//         .host = "invalid_host",
//         .port = 5432,
//         .database_name = "invalid_db",
//         .username = "invalid_user",
//         .password = "invalid_pass",
//         .pool_size = 1
//     };

//     auto connectionString =
//         "postgresql://" + invalidConfig.username + ":" + invalidConfig.password +
//         "@" + invalidConfig.host + ":" + std::to_string(invalidConfig.port) +
//         "/" + invalidConfig.database_name;

// auto& service = nuansa::services::user::UserService::GetInstance();
//     // Initialize with invalid config and verify it throws
//     EXPECT_THROW({
//         connectionPool.Initialize(connectionString, invalidConfig.pool_size);
//     }, nuansa::utils::error::DatabaseCreateConnectionError);

//     // Attempt to register user - should fail gracefully

//     EXPECT_NO_THROW(service.Initialize());

//     nuansa::models::User testUser("testuser", "test@example.com", "Password123#*",
//         nuansa::utils::crypto::GenerateRandomSalt());
//     EXPECT_FALSE(service.RegisterUser(testUser));

//     // Restore valid configuration for cleanup
//     auto& config = nuansa::config::Config::GetInstance();
//     auto validConfig = config.GetDatabaseConfig();

//     connectionString =
//         "postgresql://" + validConfig.username + ":" + validConfig.password +
//         "@" + validConfig.host + ":" + std::to_string(validConfig.port) +
//         "/" + validConfig.database_name;

//     // Reinitialize pool with valid config
//     EXPECT_NO_THROW(connectionPool.Initialize(connectionString, validConfig.pool_size));
// }

//Test getting user by username
TEST_F(UserServiceTest, GetUserByUsername) {
    EXPECT_TRUE(CreateTestUser());

    const auto user = GetTestUser();
    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->GetUsername(), "testuser");
    EXPECT_EQ(user->GetEmail(), "test@example.com");
}

// Test getting non-existent user by username
TEST_F(UserServiceTest, GetUserByUsernameNotFound) {
    auto &service = nuansa::services::user::UserService::GetInstance();
    const auto user = service.GetUserByUsername("nonexistent");
    EXPECT_FALSE(user.has_value());
}

// Test getting user by email
TEST_F(UserServiceTest, GetUserByEmail) {
    auto &service = nuansa::services::user::UserService::GetInstance();
    CreateTestUser("testuser", "test@example.com", "Password123#*");

    const auto user = service.GetUserByEmail("test@example.com");
    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->GetUsername(), "testuser");
    EXPECT_EQ(user->GetEmail(), "test@example.com");
}

// Test getting non-existent user by email
TEST_F(UserServiceTest, GetUserByEmailNotFound) {
    auto &service = nuansa::services::user::UserService::GetInstance();
    const auto user = service.GetUserByEmail("nonexistent@example.com");
    EXPECT_FALSE(user.has_value());
}

// Test email taken check
TEST_F(UserServiceTest, IsEmailTaken) {
    EXPECT_TRUE(CreateTestUser());

    const auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_TRUE(service.IsEmailTaken("test@example.com"));
    EXPECT_FALSE(service.IsEmailTaken("other@example.com"));
}

// Test username taken check
TEST_F(UserServiceTest, IsUsernameTaken) {
    EXPECT_TRUE(CreateTestUser());

    const auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_TRUE(service.IsUsernameTaken("testuser"));
    EXPECT_FALSE(service.IsUsernameTaken("otheruser"));
}


// Test database error handling for email check
TEST_F(UserServiceTest, IsEmailTakenDatabaseError) {
    const nuansa::config::DatabaseConfig invalidConfig{
        .host = "invalid_host",
        .port = 5432,
        .database_name = "invalid_db",
        .username = "invalid_user",
        .password = "invalid_pass",
        .pool_size = 1
    };

    nuansa::config::Config::GetInstance().SetDatabaseConfig(invalidConfig);
    const auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_FALSE(service.IsEmailTaken("test@example.com"));
}

// Test database error handling for username check
TEST_F(UserServiceTest, IsUsernameTakenDatabaseError) {
    const nuansa::config::DatabaseConfig invalidConfig{
        .host = "invalid_host",
        .port = 5432,
        .database_name = "invalid_db",
        .username = "invalid_user",
        .password = "invalid_pass",
        .pool_size = 1
    };

    nuansa::config::Config::GetInstance().SetDatabaseConfig(invalidConfig);
    const auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_FALSE(service.IsUsernameTaken("testuser"));
}

// Test invalid username formats
TEST_F(UserServiceTest, RegisterUserInvalidUsername) {
    nuansa::services::user::UserService::GetInstance();

    // Test empty username
    EXPECT_FALSE(CreateTestUser("", "test@example.com", "Password123#*"));

    // Test username too short
    EXPECT_FALSE(CreateTestUser("ab", "test@example.com", "Password123#*"));

    // Test username too long
    const std::string longUsername(33, 'a'); // 33 characters
    EXPECT_FALSE(CreateTestUser(longUsername, "test@example.com", "Password123#*"));

    // Test username with invalid characters
    EXPECT_FALSE(CreateTestUser("test@user", "test@example.com", "Password123#*"));
    EXPECT_FALSE(CreateTestUser("test user", "test@example.com", "Password123#*"));
}

// Test invalid email formats
TEST_F(UserServiceTest, RegisterUserInvalidEmail) {
    nuansa::services::user::UserService::GetInstance();

    // Test empty email
    EXPECT_FALSE(CreateTestUser("testuser1", "", "Password123#*"));

    // Test invalid email formats
    EXPECT_FALSE(CreateTestUser("testuser2", "notemail", "Password123#*"));
    EXPECT_FALSE(CreateTestUser("testuser3", "@example.com", "Password123#*"));
    EXPECT_FALSE(CreateTestUser("testuser4", "test@", "Password123#*"));
    EXPECT_FALSE(CreateTestUser("testuser5", "test@.com", "Password123#*"));
}

// Test invalid password requirements
TEST_F(UserServiceTest, RegisterUserInvalidPassword) {
    nuansa::services::user::UserService::GetInstance();

    // Test empty password
    EXPECT_FALSE(CreateTestUser("testuser1", "test@example.com", ""));

    // Test password too short
    EXPECT_FALSE(CreateTestUser("testuser2", "test@example.com", "pass"));

    // Test password too long
    const std::string longPassword(129, 'a'); // 129 characters
    EXPECT_FALSE(CreateTestUser("testuser3", "test@example.com", longPassword));
}

// Test user authentication
TEST_F(UserServiceTest, AuthenticateUser) {
    auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_TRUE(CreateTestUser());

    // Test successful authentication
    EXPECT_TRUE(service.AuthenticateUser("testuser", "Password123#*"));

    // Test wrong password
    EXPECT_FALSE(service.AuthenticateUser("testuser", "WrongPassword123#*"));

    // Test non-existent user
    EXPECT_FALSE(service.AuthenticateUser("nonexistent", "Password123#*"));
}

// Test user update operations
TEST_F(UserServiceTest, UpdateUser) {
    auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_TRUE(CreateTestUser());

    // Test email update
    EXPECT_TRUE(service.UpdateUserEmail("testuser", "newemail@example.com"));
    const auto user = GetTestUser();
    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->GetEmail(), "newemail@example.com");

    // Test password update
    EXPECT_TRUE(service.UpdateUserPassword("testuser", "NewPassword123#*"));
    EXPECT_TRUE(service.AuthenticateUser("testuser", "NewPassword123#*"));
    EXPECT_FALSE(service.AuthenticateUser("testuser", "Password123#*"));
}

// Test user deletion
TEST_F(UserServiceTest, DeleteUser) {
    auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_TRUE(CreateTestUser());

    EXPECT_TRUE(service.DeleteUser("testuser"));

    // Verify user is deleted
    EXPECT_FALSE(GetTestUser().has_value());
    EXPECT_FALSE(service.IsUsernameTaken("testuser"));
    EXPECT_FALSE(service.IsEmailTaken("test@example.com"));

    // Test deleting non-existent user
    EXPECT_FALSE(service.DeleteUser("nonexistent"));
}

// Test concurrent user operations
TEST_F(UserServiceTest, ConcurrentOperations) {
    auto &service = nuansa::services::user::UserService::GetInstance();

    std::vector<std::thread> threads;
    constexpr int numThreads = 10;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&service, i]() {
            const std::string username = "user" + std::to_string(i);
            const std::string email = "user" + std::to_string(i) + "@example.com";

            EXPECT_TRUE(service.CreateUser(nuansa::models::User(username, email, "Password123#*", "")));
            EXPECT_TRUE(service.AuthenticateUser(username, "Password123#*"));

            const auto user = service.GetUserByUsername(username);
            EXPECT_TRUE(user.has_value());
        });
    }

    for (auto &thread: threads) {
        thread.join();
    }
}

// Test transaction rollback
TEST_F(UserServiceTest, TransactionRollback) {
    const auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_TRUE(CreateTestUser());

    // This should fail and not affect the database state
    EXPECT_FALSE(CreateTestUser("testuser2", "test@example.com", "Password123#*"));

    // Verify the second user wasn't partially created
    EXPECT_FALSE(GetTestUser("testuser2").has_value());
    EXPECT_FALSE(service.IsUsernameTaken("testuser2"));
}

// Test invalid email update
TEST_F(UserServiceTest, UpdateUserEmailInvalid) {
    auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_TRUE(CreateTestUser());

    // Test invalid email format
    EXPECT_FALSE(service.UpdateUserEmail("testuser", "invalid-email"));

    // Test updating non-existent user
    EXPECT_FALSE(service.UpdateUserEmail("nonexistent", "new@example.com"));
}

// Test invalid password update
TEST_F(UserServiceTest, UpdateUserPasswordInvalid) {
    auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_TRUE(CreateTestUser());

    // Test empty password
    EXPECT_FALSE(service.UpdateUserPassword("testuser", ""));

    // Test password too short
    EXPECT_FALSE(service.UpdateUserPassword("testuser", "short"));

    // Test password too long
    const std::string longPassword(129, 'a'); // 129 characters
    EXPECT_FALSE(service.UpdateUserPassword("testuser", longPassword));

    // Test updating non-existent user
    EXPECT_FALSE(service.UpdateUserPassword("nonexistent", "NewPassword123#*"));
}

// Test CreateUser method directly
TEST_F(UserServiceTest, CreateUserDirect) {
    auto &service = nuansa::services::user::UserService::GetInstance();
    const nuansa::models::User user("testuser", "test@example.com", "HashedPassword123#*", "");

    service.CreateUser(user);

    const auto fetchedUser = GetTestUser();
    ASSERT_TRUE(fetchedUser.has_value());
    EXPECT_EQ(fetchedUser->GetUsername(), user.GetUsername());
    EXPECT_EQ(fetchedUser->GetEmail(), user.GetEmail());
}

// Test password hash validation
TEST_F(UserServiceTest, PasswordHashValidation) {
    nuansa::services::user::UserService::GetInstance();
    EXPECT_TRUE(CreateTestUser());

    // Test empty password (should only validate length requirements)
    EXPECT_TRUE(nuansa::utils::Validation::ValidatePassword("ValidPassword123#*"));
    EXPECT_FALSE(nuansa::utils::Validation::ValidatePassword("Short"));

    // Test valid password format but invalid credentials
    const auto user = GetTestUser();
    ASSERT_TRUE(user.has_value());
    EXPECT_FALSE(nuansa::utils::Validation::ValidatePassword("WrongPassword#*"));
    EXPECT_TRUE(nuansa::utils::Validation::ValidatePassword("Password123#*"));
}

// Test database connection errors for all operations
TEST_F(UserServiceTest, DatabaseConnectionErrors) {
    auto &service = nuansa::services::user::UserService::GetInstance();

    // Setup invalid database configuration
    const nuansa::config::DatabaseConfig invalidConfig{
        .host = "invalid_host",
        .port = 5432,
        .database_name = "invalid_db",
        .username = "invalid_user",
        .password = "invalid_pass",
        .pool_size = 1
    };

    nuansa::config::Config::GetInstance().SetDatabaseConfig(invalidConfig);

    // Test all database operations with invalid connection
    EXPECT_FALSE(service.UpdateUserEmail("testuser", "new@example.com"));
    EXPECT_FALSE(service.UpdateUserPassword("testuser", "NewPassword123#*"));
    EXPECT_FALSE(service.DeleteUser("testuser"));
    EXPECT_FALSE(GetTestUser().has_value());
    EXPECT_FALSE(service.GetUserByEmail("test@example.com").has_value());
}

// Test concurrent user updates
TEST_F(UserServiceTest, ConcurrentUserUpdates) {
    auto &service = nuansa::services::user::UserService::GetInstance();
    EXPECT_TRUE(CreateTestUser());

    std::vector<std::thread> threads;
    constexpr int numThreads = 5;
    std::mutex testMutex;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&service, &testMutex, i]() {
            const std::string newEmail = "user" + std::to_string(i) + "@example.com";
            const std::string newPassword = "Password" + std::to_string(i) + "#*";

            // Use a mutex to prevent race conditions during updates
            {
                std::lock_guard<std::mutex> lock(testMutex);
                EXPECT_TRUE(service.UpdateUserEmail("testuser", newEmail));
                EXPECT_TRUE(service.UpdateUserPassword("testuser", newPassword));
            }

            // Verify the user can still be authenticated after update
            EXPECT_TRUE(service.AuthenticateUser("testuser", newPassword));
        });
    }

    for (auto &thread: threads) {
        thread.join();
    }

    // Verify user still exists and is valid
    const auto user = GetTestUser();
    ASSERT_TRUE(user.has_value());
}

// Test hex conversion utilities
TEST_F(UserServiceTest, HexConversion) {
    auto &service = nuansa::services::user::UserService::GetInstance();

    // Create a user to test password hashing
    EXPECT_TRUE(CreateTestUser());

    // Get the user to verify hash format
    const auto user = GetTestUser();
    ASSERT_TRUE(user.has_value());

    // Get the password hash and salt
    const std::string &storedHash = user->GetPasswordHash();
    const std::string &storedSalt = user->GetSalt();

    // Verify salt is not empty
    EXPECT_FALSE(storedSalt.empty());

    // Verify hash is not empty
    EXPECT_FALSE(storedHash.empty());

    // Verify we can authenticate with the original password
    EXPECT_TRUE(service.AuthenticateUser("testuser", "Password123#*"));

    // Verify authentication fails with wrong password
    EXPECT_FALSE(service.AuthenticateUser("testuser", "WrongPassword123#*"));
}
