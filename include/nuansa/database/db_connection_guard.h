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
                ConnectionPool::GetInstance().ReturnConnection(std::move(conn_));
            }
        }

        template<typename Func>
        auto ExecuteWithRetry(Func &&func) -> decltype(func(std::declval<pqxx::connection &>())) {
            const auto &[maxRetries, initialDelay, maxDelay, backoffMultiplier] =
                    ConnectionPool::GetInstance().GetRetryConfig();
            auto delay = initialDelay;
            decltype(func(*conn_)) result;

            // Retry loop: continues on transient errors, exits on success or fatal errors
            for (size_t attempt = 0; attempt < maxRetries; ++attempt) {
                try {
                    if (!conn_) {
                        throw std::runtime_error("No valid connection");
                    }

                    try {
                        conn_->cancel_query();
                    } catch (...) {
                        // Ignore cancel errors
                    }
                    result = func(*conn_);
                } catch (const pqxx::broken_connection &e) {
                    throw utils::exception::DatabaseBrokenConnectionException(e.what());
                } catch (const std::exception &e) {
                    if (attempt + 1 == maxRetries || !IsTransientError(e)) {
                        LOG_ERROR << "Database operation failed after " << (attempt + 1) << " attempts: " << e.what();
                        throw utils::exception::NonTransientDatabaseException(e.what());
                    }

                    // Only for transient errors:
                    std::this_thread::sleep_for(delay);
                    delay = std::min(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::duration<double, std::milli>(
                                delay.count() * backoffMultiplier
                            )
                        ),
                        maxDelay
                    );
                }

                if (result) {
                    return result;
                }
                // Loop will continue to next iteration for transient errors
            }

            throw utils::exception::Exception("Unexpected error in ExecuteWithRetry");
        }

        static bool IsTransientError(const std::exception &e) {
            const std::string error = e.what();

            // Common PostgreSQL transient error codes
            return error.find("connection lost") != std::string::npos ||
                   error.find("server closed the connection unexpectedly") != std::string::npos ||
                   error.find("timeout") != std::string::npos ||
                   error.find("deadlock") != std::string::npos ||
                   error.find("connection reset by peer") != std::string::npos;
        }

    private:
        std::shared_ptr<pqxx::connection> conn_;
    };
} // namespace nuansa::database

#endif // NUANSA_DATABASE_DB_CONNECTION_GUARD_H
