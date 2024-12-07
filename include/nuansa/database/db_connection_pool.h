#ifndef NUANSA_DATABASE_DB_CONNECTION_POOL_H
#define NUANSA_DATABASE_DB_CONNECTION_POOL_H

#include "nuansa/utils/pch.h"
#include "nuansa/utils/pattern/circuit_breaker.h"
#include "nuansa/utils/exception/database_exception.h"

namespace nuansa::database {
    class ConnectionPool {
    public:
        struct RetryConfig {
            size_t maxRetries = 3;
            std::chrono::milliseconds initialDelay = std::chrono::milliseconds(100);
            std::chrono::milliseconds maxDelay = std::chrono::milliseconds(1000);
            double backoffMultiplier = 2.0;
        };

        const RetryConfig &GetRetryConfig() const { return retryConfig_; }

        static ConnectionPool &GetInstance();

        void Initialize(const std::string &connectionString, const size_t poolSize = 10);

        std::shared_ptr<pqxx::connection> AcquireConnection(
            const std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

        // Add retry configuration
        void SetRetryConfig(const RetryConfig &config) { retryConfig_ = config; }

        void ReturnConnection(std::shared_ptr<pqxx::connection> conn);

        void Shutdown();

        bool IsInitialized() const;

        // Add circuit breaker configuration
        utils::pattern::CircuitBreaker &GetCircuitBreaker() { return circuitBreaker_; }

        ConnectionPool(const ConnectionPool &) = delete;

        ConnectionPool &operator=(const ConnectionPool &) = delete;

    private:
        ConnectionPool() = default;

        ~ConnectionPool();

        static constexpr auto DEFAULT_TIMEOUT = std::chrono::milliseconds(5000);

        std::queue<std::shared_ptr<pqxx::connection> > connections_;
        std::mutex mutex_;
        std::condition_variable connectionAvailable_;
        std::string connectionString_;
        std::string fallbackConnectionString_;
        size_t poolSize_ = 10;
        size_t maxPoolSize_ = 20; // Maximum number of connections allowed
        std::atomic<size_t> activeConnections_{0};
        bool initialized_ = false;
        RetryConfig retryConfig_;

        utils::pattern::CircuitBreaker circuitBreaker_;

        std::shared_ptr<pqxx::connection> CreateConnection();

        std::shared_ptr<pqxx::connection> GetFallbackConnection();

        void SetFallbackConnectionString(const std::string &connectionString);
    };
} // namespace nuansa::database

#endif // NUANSA_DATABASE_DB_CONNECTION_POOL_H
