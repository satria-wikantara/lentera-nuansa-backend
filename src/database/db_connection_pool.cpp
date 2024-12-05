#include "nuansa/pch/pch.h"

#include "nuansa/database/db_connection_pool.h"
#include "nuansa/utils/errors/database_error.h"

namespace nuansa::database {
    ConnectionPool &ConnectionPool::GetInstance() {
        static ConnectionPool instance;
        return instance;
    }

    void ConnectionPool::Initialize(const std::string &connectionString, size_t poolSize) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (initialized_) {
            Shutdown();
        }

        connectionString_ = connectionString;
        poolSize_ = poolSize;
        maxPoolSize_ = poolSize * 2;
        activeConnections_ = 0;

        // Initialize circuit breaker
        circuitBreaker_.Initialize(utils::patterns::CircuitBreakerSettings{
            .failureThreshold = 5,
            .successThreshold = 2,
            .resetTimeout = std::chrono::seconds(30),
            .timeout = std::chrono::seconds(10)
        });
        circuitBreaker_.Reset();

        BOOST_LOG_TRIVIAL(info) << "Initializing connection pool with size " << poolSize_;

        bool initializationFailed = false;
        std::string errorMessage;

        // Try to create at least one connection
        try {
            BOOST_LOG_TRIVIAL(info) << "Creating initial connection";
            auto conn = CreateConnection();
            if (conn) {
                BOOST_LOG_TRIVIAL(info) << "Pushing initial connection to pool";
                connections_.push(std::move(conn));
                activeConnections_++;
            }
            BOOST_LOG_TRIVIAL(info) << "Initial connection created";
        } catch (const std::exception &e) {
            initializationFailed = true;
            errorMessage = e.what();
            BOOST_LOG_TRIVIAL(error) << "Failed to create initial connection: " << errorMessage;
        }

        // If we couldn't create even one connection, throw an exception
        if (initializationFailed || connections_.empty()) {
            BOOST_LOG_TRIVIAL(error) << "Shutting down connection pool due to initialization failure";
            Shutdown();
            throw nuansa::utils::errors::DatabaseCreateConnectionError(
                "Failed to initialize connection pool: " + errorMessage
            );
        }

        // Try to create remaining connections
        while (connections_.size() < poolSize_) {
            try {
                auto conn = CreateConnection();
                if (conn) {
                    connections_.push(std::move(conn));
                    activeConnections_++;
                }
            } catch (const std::exception &e) {
                BOOST_LOG_TRIVIAL(warning) << "Failed to create additional connection: " << e.what();
                break; // Stop trying to create more connections
            }
        }

        initialized_ = true;
        BOOST_LOG_TRIVIAL(info) << "Connection pool initialized with " << connections_.size() << " connections";
    }

    std::shared_ptr<pqxx::connection> ConnectionPool::CreateConnection() {
        try {
            BOOST_LOG_TRIVIAL(info) << "Creating connection with connection string: " << connectionString_;
            return std::make_shared<pqxx::connection>(connectionString_);
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Unexpected error creating database connection: " << e.what();
            throw nuansa::utils::errors::DatabaseCreateConnectionError(e.what());
        }
    }

    bool ConnectionPool::IsTransientError(const std::exception &e) {
        const std::string error = e.what();

        // Common PostgreSQL transient error codes
        return error.find("connection lost") != std::string::npos ||
               error.find("server closed the connection unexpectedly") != std::string::npos ||
               error.find("timeout") != std::string::npos ||
               error.find("connection reset by peer") != std::string::npos;
    }

    void ConnectionPool::Shutdown() {
        std::unique_lock<std::mutex> lock(mutex_);
        BOOST_LOG_TRIVIAL(info) << "Shutting down connection pool";
        // First mark as not initialized to prevent new connection requests
        initialized_ = false;
        BOOST_LOG_TRIVIAL(info) << "Connection pool marked as not initialized";
        // Wake up any waiting threads before clearing connections
        connectionAvailable_.notify_all();
        BOOST_LOG_TRIVIAL(info) << "Notified all waiting threads";

        // Give waiting threads a chance to wake up and exit
        lock.unlock();
        BOOST_LOG_TRIVIAL(info) << "Unlocked mutex";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lock.lock();
        BOOST_LOG_TRIVIAL(info) << "Locked mutex";

        // Clear all connections
        while (!connections_.empty()) {
            BOOST_LOG_TRIVIAL(info) << "Clearing connections";
            auto conn = std::move(connections_.front());
            connections_.pop();
            try {
                BOOST_LOG_TRIVIAL(info) << "Resetting connection";
                conn.reset();
            } catch (const std::exception &e) {
                BOOST_LOG_TRIVIAL(warning) << "Error closing connection during shutdown: " << e.what();
            }
        }

        activeConnections_ = 0;

        // Final notification for any remaining waiters
        connectionAvailable_.notify_all();

        BOOST_LOG_TRIVIAL(info) << "Connection pool shut down";
    }

    bool ConnectionPool::IsInitialized() const {
        return initialized_;
    }

    std::shared_ptr<pqxx::connection> ConnectionPool::CreateConnectionWithRetry() {
        return GetCircuitBreaker().Execute([this]() -> std::shared_ptr<pqxx::connection> {
            const auto &config = retryConfig_;
            auto delay = config.initialDelay;

            for (size_t attempt = 1; attempt <= config.maxRetries; ++attempt) {
                try {
                    return CreateConnection();
                } catch (const std::exception &e) {
                    if (attempt == config.maxRetries || !IsTransientError(e)) {
                        BOOST_LOG_TRIVIAL(error) << "Failed to create connection after "
                                           << attempt << " attempts: " << e.what();
                        throw;
                    }

                    BOOST_LOG_TRIVIAL(warning) << "Connection attempt " << attempt
                                         << " failed: " << e.what()
                                         << ". Retrying in " << delay.count() << "ms";

                    std::this_thread::sleep_for(delay);
                    auto newDelay = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::duration<double, std::milli>(
                            delay.count() * config.backoffMultiplier
                        )
                    );
                    delay = newDelay < config.maxDelay ? newDelay : config.maxDelay;
                }
            }
            throw std::runtime_error("Unexpected error in CreateConnectionWithRetry");
        });
    }

    std::shared_ptr<pqxx::connection> ConnectionPool::AcquireConnection(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!initialized_) {
            BOOST_LOG_TRIVIAL(error) << "Cannot acquire connection: pool is not initialized";
            throw std::runtime_error("Connection pool is not initialized");
        }

        BOOST_LOG_TRIVIAL(info) << "Acquiring connection from pool";

        // Wait for a connection with timeout
        auto waitResult = connectionAvailable_.wait_for(lock, timeout, [this] {
            return !initialized_ || !connections_.empty() || activeConnections_ < maxPoolSize_;
        });

        if (!initialized_) {
            throw std::runtime_error("Connection pool was shut down while waiting");
        }

        if (!waitResult) {
            BOOST_LOG_TRIVIAL(error) << "Timeout waiting for available connection";
            throw std::runtime_error("Timeout waiting for database connection");
        }

        std::shared_ptr<pqxx::connection> conn;

        if (!connections_.empty()) {
            conn = connections_.front();
            connections_.pop();
        } else if (activeConnections_ < maxPoolSize_) {
            try {
                BOOST_LOG_TRIVIAL(warning) << "Creating new connection";
                conn = CreateConnection();
                activeConnections_++;
            } catch (const std::exception &e) {
                BOOST_LOG_TRIVIAL(error) << "Failed to create new connection: " << e.what();
                // Notify other waiting threads that a connection attempt failed
                connectionAvailable_.notify_one();
                throw;
            }
        }

        if (!conn || !conn->is_open()) {
            activeConnections_--;
            connectionAvailable_.notify_one();
            throw std::runtime_error("Failed to acquire valid database connection");
        }

        return conn;
    }

    std::shared_ptr<pqxx::connection> ConnectionPool::GetFallbackConnection() {
        BOOST_LOG_TRIVIAL(info) << "Getting fallback connection";
        try {
            // First try to use a dedicated read-only replica if configured
            if (!fallbackConnectionString_.empty()) {
                BOOST_LOG_TRIVIAL(info) << "Using dedicated fallback connection string";
                return std::make_shared<pqxx::connection>(fallbackConnectionString_);
            }

            // Create a new connection with increased timeout
            // Parse the existing connection string to properly add parameters
            std::string fallbackConfig;
            if (connectionString_.find("?") != std::string::npos) {
                // Connection string already has parameters
                fallbackConfig = connectionString_ + "&connect_timeout=10";
            } else {
                // No existing parameters
                fallbackConfig = connectionString_ + "?connect_timeout=10";
            }

            BOOST_LOG_TRIVIAL(info) << "Creating new fallback connection";
            auto conn = std::make_shared<pqxx::connection>(fallbackConfig);
            conn->set_client_encoding("UTF8");

            return conn;
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Failed to create fallback connection: " << e.what();
            throw nuansa::utils::errors::DatabaseCreateConnectionError(
                "No fallback connection available: " + std::string(e.what())
            );
        }
    }

    ConnectionPool::~ConnectionPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) {
            connections_.pop();
        }
    }

    void ConnectionPool::ReturnConnection(std::shared_ptr<pqxx::connection> conn) {
        if (!conn) return;

        std::lock_guard<std::mutex> lock(mutex_);

        try {
            if (conn->is_open()) {
                try {
                    conn->cancel_query();
                } catch (...) {
                    // Ignore cancel errors
                }

                connections_.push(std::move(conn));
                connectionAvailable_.notify_one();
                BOOST_LOG_TRIVIAL(debug) << "Connection returned to pool. Pool size: " << connections_.size();
            } else {
                activeConnections_--;
                BOOST_LOG_TRIVIAL(warning) << "Discarding dead connection";
                connectionAvailable_.notify_one();
            }
        } catch (const std::exception &e) {
            activeConnections_--;
            BOOST_LOG_TRIVIAL(error) << "Error returning connection: " << e.what();
            connectionAvailable_.notify_one();
        }
    }

    void ConnectionPool::SetFallbackConnectionString(const std::string &connectionString) {
        std::lock_guard<std::mutex> lock(mutex_);
        fallbackConnectionString_ = connectionString;
    }
} // namespace App::Database
