//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_PATTERNS_CIRCUIT_BREAKER_H
#define NUANSA_UTILS_PATTERNS_CIRCUIT_BREAKER_H

#include <exception>
#include <future>
#include <mutex>
#include <__mutex/mutex.h>

#include "nuansa/pch/pch.h"
#include "nuansa/utils/errors/circuit_breaker_error.h"

namespace nuansa::utils::patterns {
	struct CircuitBreakerSettings {
		size_t failureThreshold{5}; // Number of failures before opening the circuit.
		size_t successThreshold{2}; // Number of consecutive successes to close the circuit breaker.
		std::chrono::seconds resetTimeout{std::chrono::seconds(30)}; // Time to wait before attemting to recover.
		std::chrono::milliseconds timeout{std::chrono::seconds(10)}; // Operation timeout.
	};

	class CircuitBreaker {
	public:
		enum class State {
			// Normal operation. The circuit is closed and requests are allowed
			CLOSED,
			// Failing state, fast fail. The circuit is open and requests are blocked.
			OPEN,
			// Half-open state, recovering.
			// The circuit is half-open and requests are allowed with a limited number of requests
			HALF_OPEN
		};

		struct Metrics {
			size_t totalCalls{0};
			size_t successCalls{0};
			size_t failedCalls{0};
			size_t timeouts{0};
			std::chrono::milliseconds averageResponseTime{0};
		};

		explicit CircuitBreaker(CircuitBreakerSettings settings = CircuitBreakerSettings{})
			: settings_(settings) {
		}

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

		const Metrics &GetMetrics() const {
			std::lock_guard<std::mutex> lock(mutex_);
			return metrics_;
		}

		void CheckState() {
			std::lock_guard<std::mutex> lock(mutex_);

			if (state_ == State::OPEN) {
				auto elapsed = std::chrono::steady_clock::now() - lastFailureTime_;
				if (elapsed > settings_.resetTimout) {
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

		template<typename Func>
		auto Execute(Func &&func) ->
			typename std::enable_if<!std::is_void<decltype(func())>::value,
				(func())>::type {
			CheckState();

			if (state == State::OPEN) {
				throw utils::errors::CircuitBreakerOpenError("Circuit breaker is OPEN");
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
			typename std::enable_if<std::is_void<decltype(func())>::value, void>::type {
			CheckState();

			if (state_ == State::OPEN) {
				throw utils::errors::CircuitBreakerOpenError("Circuit breaker is OPEN");
			}

			try {
				std::forward<Func>(func)();
				RecordSuccess();
			} catch (const std::exception &e) {
				RecordFailure();
				throw;
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
				throw utils::errors::CircuitBreakerTimeoutError("Operation timed out");
			}

			worker.join();
			return future.get();
		}

		void UpdateMetrics(std::chrono::milliseconds responseTime, bool success) {
			std::lock_guard<std::mutex> lock(mutex_);

			metrics_.totalCalls++;
			if (success) {
				metrics_.successCalls++;
			} else {
				metrics_.failedCalls++;
			}

			// Update average response time
			auto total = metrics_.averageResponseTime * (metrics_.totalCalls -1);
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


#endif //NUANSA_UTILS_PATTERNS_CIRCUIT_BREAKER_H