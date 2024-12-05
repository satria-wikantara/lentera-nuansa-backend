//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_ERRORS_CIRCUIT_BREAKER_ERROR_H
#define NUANSA_UTILS_ERRORS_CIRCUIT_BREAKER_ERROR_H

#include "nuansa/pch/pch.h"

namespace nuansa::utils::errors {
	class CircuitBreakerError : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	class CircuitBreakerOpenError : public CircuitBreakerError {
	public:
		explicit CircuitBreakerOpenError(const std::string &message)
			: CircuitBreakerError("Circuit Breaker Open: " + message) {
		}
	};

	class CircuitBreakerTimeoutError : public CircuitBreakerError {
	public:
		explicit CircuitBreakerTimeoutError(const std::string &message)
			: CircuitBreakerError("Circuit Breaker Timeout: " + message) {
		}
	};

	class CircuitBreakerCloseError : public CircuitBreakerError {
	public:
		explicit CircuitBreakerCloseError(const std::string &message)
			: CircuitBreakerError("Circuit Breaker Close: " + message) {
		}
	};
}

#endif //NUANSA_UTILS_ERRORS_CIRCUIT_BREAKER_ERROR_H
