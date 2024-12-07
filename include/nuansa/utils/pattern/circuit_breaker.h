#ifndef NUANSA_UTILS_PATTERN_CIRCUIT_BREAKER_H
#define NUANSA_UTILS_PATTERN_CIRCUIT_BREAKER_H

#include "nuansa/pch/pch.h"
#include "nuansa/utils/exception/circuit_breaker_exception.h"

namespace nuansa::utils::pattern {
    struct CircuitBreakerSettings {
        size_t failureThreshold{5}; // Number of failures before opening the circuit.
        size_t successThreshold{2}; // Number of consecutive successes to close the circuit breaker.
        std::chrono::seconds resetTimeout{std::chrono::seconds(30)}; // Time to wait before attemting to recover.
        std::chrono::seconds timeout{std::chrono::seconds(10)}; // Operation timeout.
    };

    /**
     * @brief Implementation of the Circuit Breaker pattern
     *
     * The Circuit Breaker pattern prevents cascading failures in distributed
     * systems by detecting failures and encapsulating the logic of preventing
     * a failure from constantly recurring.
     *
     * Usage example:
     * @code
     * CircuitBreaker breaker;
     * try {
     *     auto result = breaker.Execute([](){ return makeHttpCall(); });
     * } catch (const CircuitBreakerOpenException& e) {
     *     // Handle circuit open state
     * }
     * @endcode
     */
    class CircuitBreaker {
    public:
        enum class State {
            CLOSED, // Normal operation. The circuit is closed and requests are allowed.
            OPEN, // Failing state, fast fail. The circuit is open and requests are blocked.
            HALF_OPEN
            // Half-open state, recovering. The circuit is half-open and requests are allowed with a limited number of requests.
        };

        static CircuitBreaker &GetInstance() {
            static CircuitBreaker instance;
            return instance;
        }

        bool IsInitialized() const {
            return initialized_;
        }

        void Reset() {
            std::lock_guard<std::mutex> lock(mutex_);
            failureCount_ = 0;
            successCount_ = 0;
            state_ = State::CLOSED;
            lastStateChange_ = std::chrono::steady_clock::now();
        }

        void Initialize(const CircuitBreakerSettings &settings = CircuitBreakerSettings{}) {
            std::lock_guard<std::mutex> lock(mutex_);

            if (initialized_) {
                return;
            }

            settings_ = settings;
            initialized_ = true;
        }

        struct Metrics {
            size_t totalCalls{0};
            size_t successfulCalls{0};
            size_t failedCalls{0};
            size_t timeouts{0};
            std::chrono::milliseconds averageResponseTime{0};
        };

        const Metrics &GetMetrics() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return metrics_;
        }

        bool IsOpen() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return state_ == State::OPEN;
        }

        explicit CircuitBreaker(CircuitBreakerSettings settings = CircuitBreakerSettings{})
            : settings_{std::move(settings)} {
        }

        template<typename Func>
        auto Execute(Func &&func) ->
            typename std::enable_if<!std::is_void<decltype(func())>::value,
                decltype(func())>::type {
            CheckState();

            if (state_ == State::OPEN) {
                throw utils::exception::CircuitBreakerOpenException("Circuit breaker is OPEN");
            }

            try {
                auto result = std::forward<Func>(func)();
                RecordSuccess();
                return result;
            } catch (const std::exception &e) {
                RecordFailure();
                throw;
            }
        }

        template<typename Func>
        auto Execute(Func &&func) ->
            typename std::enable_if<std::is_void<decltype(func())>::value,
                void>::type {
            CheckState();

            if (state_ == State::OPEN) {
                throw utils::exception::CircuitBreakerOpenException("Circuit breaker is OPEN");
            }

            try {
                std::forward<Func>(func)();
                RecordSuccess();
            } catch (const std::exception &e) {
                RecordFailure();
                throw;
            }
        }

        void CheckState() {
            std::lock_guard<std::mutex> lock(mutex_);

            if (state_ == State::OPEN) {
                auto elapsed = std::chrono::steady_clock::now() - lastFailureTime_;
                if (elapsed >= settings_.resetTimeout) {
                    state_ = State::HALF_OPEN;
                    failureCount_ = 0;
                    successCount_ = 0;
                }
            }
        }

        void RecordSuccess() {
            std::lock_guard<std::mutex> lock(mutex_);

            if (state_ == State::HALF_OPEN) {
                ++successCount_;
                if (successCount_ >= settings_.successThreshold) {
                    state_ = State::CLOSED;
                    failureCount_ = 0;
                    successCount_ = 0;
                }
            }
        }

        void RecordFailure() {
            std::lock_guard<std::mutex> lock(mutex_);

            failureCount_++;
            lastFailureTime_ = std::chrono::steady_clock::now();

            if (state_ == State::CLOSED && failureCount_ >= settings_.failureThreshold) {
                state_ = State::OPEN;
            } else if (state_ == State::HALF_OPEN) {
                state_ = State::OPEN;
            }
        }

        template<typename Func, typename... Args>
        auto ExecuteWithTimeout(Func &&func, Args &&... args) {
            std::promise<decltype(func(args...))> promise;
            auto future = promise.get_future();

            // Execute function in a separate thread
            std::thread worker([&promise, func = std::forward<Func>(func),
                    ... args = std::forward<Args>(args)]() mutable {
                    try {
                        promise.set_value(func(std::forward<Args>(args)...));
                    } catch (...) {
                        promise.set_exception(std::current_exception());
                    }
                });

            // Wait for the function to finish or timeout
            if (future.wait_for(settings_.timeout) == std::future_status::timeout) {
                worker.detach();
                throw utils::exception::CircuitBreakerTimeoutException("Operation timed out");
            }

            worker.join();
            return future.get();
        }

        void UpdateMetrics(std::chrono::milliseconds responseTime, bool success) {
            std::lock_guard<std::mutex> lock(mutex_);

            metrics_.totalCalls++;
            if (success) {
                metrics_.successfulCalls++;
            } else {
                metrics_.failedCalls++;
            }

            // Update average response time
            auto total = metrics_.averageResponseTime * (metrics_.totalCalls - 1);
            metrics_.averageResponseTime = (total + responseTime) / metrics_.totalCalls;
        }

    private:
        State state_{State::CLOSED};
        size_t failureCount_{0};
        size_t successCount_{0};
        std::chrono::steady_clock::time_point lastStateChange_;
        bool initialized_{false};
        CircuitBreakerSettings settings_;
        Metrics metrics_;
        std::chrono::steady_clock::time_point lastFailureTime_;
        mutable std::mutex mutex_;
    };
}

#endif // NUANSA_UTILS_PATTERN_CIRCUIT_BREAKER_H
