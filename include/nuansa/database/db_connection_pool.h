#ifndef NUANSA_DATABASE_DB_CONNECTION_POOL_H
#define NUANSA_DATABASE_DB_CONNECTION_POOL_H

#include "nuansa/pch/pch.h"
#include "nuansa/utils/patterns/circuit_breaker.h"

namespace nuansa::database {

class ConnectionPool {
public:
    struct RetryConfig {
        size_t maxRetries = 3;
        std::chrono::milliseconds initialDelay = std::chrono::milliseconds(100);
        std::chrono::milliseconds maxDelay = std::chrono::milliseconds(1000);
        double backoffMultiplier = 2.0;
    };

    RetryConfig& GetRetryConfig() { return retryConfig_; }
    const RetryConfig& GetRetryConfig() const { return retryConfig_; }

    static ConnectionPool& GetInstance();
    
    void Initialize(const std::string& connectionString, size_t poolSize = 10);
    std::shared_ptr<pqxx::connection> AcquireConnection(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    
    // Add retry configuration
    void SetRetryConfig(const RetryConfig& config) { retryConfig_ = config; }
    
    void ReturnConnection(std::shared_ptr<pqxx::connection> conn);
    
    bool IsTransientError(const std::exception& e);

    void Shutdown();

    bool IsInitialized() const;
    
    // Add circuit breaker configuration
    utils::patterns::CircuitBreaker& GetCircuitBreaker() { return circuitBreaker_; }
    
private:
    ConnectionPool() = default;
    ~ConnectionPool();
    
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    std::queue<std::shared_ptr<pqxx::connection>> connections_;
    std::mutex mutex_;
    std::condition_variable connectionAvailable_;
    std::string connectionString_;
    size_t poolSize_ = 10;
    size_t maxPoolSize_ = 20;  // Maximum number of connections allowed
    std::atomic<size_t> activeConnections_{0};
    bool initialized_ = false;
    RetryConfig retryConfig_;

    utils::patterns::CircuitBreaker circuitBreaker_;
    
    std::shared_ptr<pqxx::connection> CreateConnection();
    std::shared_ptr<pqxx::connection> CreateConnectionWithRetry();
    std::shared_ptr<pqxx::connection> GetFallbackConnection();
    void SetFallbackConnectionString(const std::string& connectionString);

    std::string fallbackConnectionString_;
    static constexpr auto DEFAULT_TIMEOUT = std::chrono::milliseconds(5000);
};

// RAII wrapper for connection handling with retry support
class ConnectionGuard {
public:
    explicit ConnectionGuard(std::shared_ptr<pqxx::connection> conn) : conn_(conn) {}
    ~ConnectionGuard() {
        if (conn_) {
            ConnectionPool::GetInstance().ReturnConnection(std::move(conn_));
        }
    }

    template<typename Func>
    auto ExecuteWithRetry(Func&& func) -> decltype(func(std::declval<pqxx::connection&>())) {
        const auto& retryConfig = ConnectionPool::GetInstance().GetRetryConfig();
        auto delay = retryConfig.initialDelay;
        
        for (size_t attempt = 1; attempt <= retryConfig.maxRetries; ++attempt) {
            try {
                if (!conn_) {
                    throw std::runtime_error("No valid connection");
                }

                // Cancel any pending operations before starting new transaction
                try {
                    conn_->cancel_query();
                } catch (...) {
                    // Ignore cancel errors
                }
                
                return func(*conn_);
            } catch (const pqxx::broken_connection& e) {
                throw;
            } catch (const std::exception& e) {
                if (attempt == retryConfig.maxRetries || !ConnectionPool::GetInstance().IsTransientError(e)) {
                    BOOST_LOG_TRIVIAL(error) << "Database operation failed after " 
                                           << attempt << " attempts: " << e.what();
                    throw;
                }

                std::this_thread::sleep_for(delay);
                delay = std::min(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::duration<double, std::milli>(
                            delay.count() * retryConfig.backoffMultiplier
                        )
                    ),
                    retryConfig.maxDelay
                );
            }
        }
        throw std::runtime_error("Unexpected error in ExecuteWithRetry");
    }

private:
    std::shared_ptr<pqxx::connection> conn_;
    
    bool IsTransientError(const std::exception& e) {
        const std::string error = e.what();
        return error.find("connection") != std::string::npos ||
               error.find("deadlock") != std::string::npos ||
               error.find("timeout") != std::string::npos;
    }
};

} // namespace nuansa::database

#endif // NUANSA_DATABASE_DB_CONNECTION_POOL_H