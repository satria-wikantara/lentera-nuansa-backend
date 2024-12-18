#ifndef NUANSA_DATABASE_DB_CONNECTION_GUARD_H
#define NUANSA_DATABASE_DB_CONNECTION_GUARD_H

#include "nuansa/utils/pch.h"
#include "nuansa/database/db_connection_pool.h"
#include "nuansa/utils/exception/database_exception.h"

namespace nuansa::database {
    // RAII wrapper for connection handling with retry support
    class ConnectionGuard {
    public:
        explicit ConnectionGuard(std::shared_ptr<pqxx::connection> conn) : conn_(std::move(conn)) {
        }

        ~ConnectionGuard() {
            if (conn_) {
                try {
                    ConnectionPool::GetInstance().ReturnConnection(std::move(conn_));
                } catch (...) {
                    // Just log, don't throw from destructor
                    LOG_ERROR << "Failed to return connection to pool";
                }
            }
        }

        template<typename F>
        auto ExecuteWithRetry(F&& func, int maxRetries = 3) -> decltype(func(std::declval<pqxx::connection&>())) {
            if (!conn_) {
                throw std::runtime_error("No valid connection");
            }

            for (int attempt = 1; attempt <= maxRetries; ++attempt) {
                try {
                    if (!conn_->is_open()) {
                        LOG_WARNING << "Connection closed, attempting to reconnect (attempt " << attempt << ")";
                        conn_ = ConnectionPool::GetInstance().AcquireConnection(
                            std::chrono::milliseconds(1000)
                        );
                        if (!conn_ || !conn_->is_open()) {
                            throw std::runtime_error("Failed to acquire new connection");
                        }
                    }

                    return func(*conn_);

                } catch (const pqxx::broken_connection& e) {
                    LOG_ERROR << "Database connection broken: " << e.what();
                    if (attempt == maxRetries) throw;
                    
                    // Get new connection for retry
                    conn_ = ConnectionPool::GetInstance().AcquireConnection(
                        std::chrono::milliseconds(1000)
                    );
                    
                } catch (const pqxx::sql_error& e) {
                    LOG_ERROR << "SQL error: " << e.what() << " (attempt " << attempt << "/" << maxRetries << ")";
                    if (attempt == maxRetries) throw;
                    
                    // For SQL errors, check if we should retry
                    if (e.sqlstate() == "40001" || // serialization_failure
                        e.sqlstate() == "40P01") { // deadlock_detected
                        std::this_thread::sleep_for(std::chrono::milliseconds(100 * attempt));
                        continue;
                    }
                    throw; // Don't retry other SQL errors
                    
                } catch (const std::exception& e) {
                    LOG_ERROR << "Database error: " << e.what() << " (attempt " << attempt << "/" << maxRetries << ")";
                    if (attempt == maxRetries) {
                        throw std::runtime_error("Unexpected error in ExecuteWithRetry: " + std::string(e.what()));
                    }
                }

                // Exponential backoff between retries
                std::this_thread::sleep_for(std::chrono::milliseconds(100 * attempt));
            }

            throw std::runtime_error("Max retries exceeded");
        }

    private:
        std::shared_ptr<pqxx::connection> conn_;
    };
} // namespace nuansa::database

#endif // NUANSA_DATABASE_DB_CONNECTION_GUARD_H
