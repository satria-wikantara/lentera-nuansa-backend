#include "nuansa/pch/pch.h"

#include "nuansa/database/db_connection_pool.h"
#include "nuansa/utils/exception/database_exception.h"

namespace nuansa::database {
    ConnectionPool &ConnectionPool::GetInstance() {
        static ConnectionPool instance;
        return instance;
    }

    void ConnectionPool::Initialize(const std::string &connectionString, const size_t poolSize) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (initialized_) {
            Shutdown();
        }

        connectionString_ = connectionString;
        poolSize_ = poolSize;
        maxPoolSize_ = poolSize * 2;
        activeConnections_ = 0;

        // Initialize circuit breaker
        circuitBreaker_.Initialize(utils::pattern::CircuitBreakerSettings{
            .failureThreshold = 5,
            .successThreshold = 2,
            .resetTimeout = std::chrono::seconds(30),
            .timeout = std::chrono::seconds(10)
        });
        circuitBreaker_.Reset();

        LOG_INFO << "Initializing connection pool with size " << poolSize_;

        bool initializationFailed = false;
        std::string errorMessage;

        // Try to create at least one connection
        try {
            LOG_INFO << "Creating initial connection";
            if (auto conn = CreateConnection()) {
                LOG_INFO << "Pushing initial connection to pool";
                connections_.push(std::move(conn));
                ++activeConnections_;
            }
            LOG_INFO << "Initial connection created";
        } catch (const std::exception &e) {
            initializationFailed = true;
            errorMessage = e.what();
            LOG_ERROR << "Failed to create initial connection: " << errorMessage;
        }

        // If we couldn't create even one connection, throw an exception
        if (initializationFailed || connections_.empty()) {
            LOG_ERROR << "Shutting down connection pool due to initialization failure";
            Shutdown();
            throw nuansa::utils::exception::DatabaseCreateConnectionException(
                "Failed to initialize connection pool: " + errorMessage
            );
        }

        // Try to create remaining connections
        while (connections_.size() < poolSize_) {
            try {
                if (auto conn = CreateConnection()) {
                    connections_.push(std::move(conn));
                    ++activeConnections_;
                }
            } catch (const std::exception &e) {
                LOG_WARNING << "Failed to create additional connection: " << e.what();
                break; // Stop trying to create more connections
            }
        }

        initialized_ = true;
        LOG_INFO << "Connection pool initialized with " << connections_.size() << " connections";
    }

    std::shared_ptr<pqxx::connection> ConnectionPool::CreateConnection() {
        try {
            LOG_INFO << "Creating connection with connection string: " << connectionString_;
            return std::make_shared<pqxx::connection>(connectionString_);
        } catch (const std::exception &e) {
            LOG_ERROR << "Unexpected error creating database connection: " << e.what();
            throw nuansa::utils::exception::DatabaseCreateConnectionException(e.what());
        }
    }

    void ConnectionPool::Shutdown() {
        std::unique_lock<std::mutex> lock(mutex_);
        LOG_INFO << "Shutting down connection pool";
        // First mark as not initialized to prevent new connection requests
        initialized_ = false;
        LOG_INFO << "Connection pool marked as not initialized";
        // Wake up any waiting threads before clearing connections
        connectionAvailable_.notify_all();
        LOG_INFO << "Notified all waiting threads";

        // Give waiting threads a chance to wake up and exit
        lock.unlock();
        LOG_INFO << "Unlocked mutex";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lock.lock();
        LOG_INFO << "Locked mutex";

        // Clear all connections
        while (!connections_.empty()) {
            LOG_INFO << "Clearing connections";
            auto conn = std::move(connections_.front());
            connections_.pop();
            try {
                LOG_INFO << "Resetting connection";
                conn.reset();
            } catch (const std::exception &e) {
                LOG_WARNING << "Error closing connection during shutdown: " << e.what();
            }
        }

        activeConnections_ = 0;

        // Final notification for any remaining waiters
        connectionAvailable_.notify_all();

        LOG_INFO << "Connection pool shut down";
    }

    bool ConnectionPool::IsInitialized() const {
        return initialized_;
    }

    std::shared_ptr<pqxx::connection> ConnectionPool::AcquireConnection(const std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!initialized_) {
            LOG_ERROR << "Cannot acquire connection: pool is not initialized";
            throw std::runtime_error("Connection pool is not initialized");
        }

        LOG_INFO << "Acquiring connection from pool";

        // Wait for a connection with timeout
        const auto waitResult = connectionAvailable_.wait_for(lock, timeout, [this] {
            return !initialized_ || !connections_.empty() || activeConnections_ < maxPoolSize_;
        });

        if (!waitResult) {
            LOG_ERROR << "Timeout waiting for available connection";
            throw std::runtime_error("Timeout waiting for database connection");
        }

        std::shared_ptr<pqxx::connection> conn;

        if (!connections_.empty()) {
            conn = connections_.front();
            connections_.pop();
        } else if (activeConnections_ < maxPoolSize_) {
            try {
                LOG_WARNING << "Creating new connection";
                conn = CreateConnection();
                ++activeConnections_;
            } catch (const std::exception &e) {
                LOG_ERROR << "Failed to create new connection: " << e.what();
                // Notify other waiting threads that a connection attempt failed
                connectionAvailable_.notify_one();
                throw;
            }
        }

        if (!conn || !conn->is_open()) {
            --activeConnections_;
            connectionAvailable_.notify_one();
            throw std::runtime_error("Failed to acquire valid database connection");
        }

        return conn;
    }

    std::shared_ptr<pqxx::connection> ConnectionPool::GetFallbackConnection() {
        LOG_INFO << "Getting fallback connection";
        try {
            // First try to use a dedicated read-only replica if configured
            if (!fallbackConnectionString_.empty()) {
                LOG_INFO << "Using dedicated fallback connection string";
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

            LOG_INFO << "Creating new fallback connection";
            auto conn = std::make_shared<pqxx::connection>(fallbackConfig);
            conn->set_client_encoding("UTF8");

            return conn;
        } catch (const std::exception &e) {
            LOG_ERROR << "Failed to create fallback connection: " << e.what();
            throw nuansa::utils::exception::DatabaseCreateConnectionException(
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
                LOG_DEBUG << "Connection returned to pool. Pool size: " << connections_.size();
            } else {
                ++activeConnections_;
                LOG_WARNING << "Discarding dead connection";
                connectionAvailable_.notify_one();
            }
        } catch (const std::exception &e) {
            ++activeConnections_;
            LOG_ERROR << "Error returning connection: " << e.what();
            connectionAvailable_.notify_one();
        }
    }

    void ConnectionPool::SetFallbackConnectionString(const std::string &connectionString) {
        std::lock_guard<std::mutex> lock(mutex_);
        fallbackConnectionString_ = connectionString;
    }
} // namespace App::Database
