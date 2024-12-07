//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_EXCEPTION_CIRCUIT_BREAKER_EXCEPTION_H
#define NUANSA_UTILS_EXCEPTION_CIRCUIT_BREAKER_EXCEPTION_H

#include "nuansa/utils/exception/exception.h"

namespace nuansa::utils::exception {
	class CircuitBreakerException : public nuansa::utils::exception::Exception {
	public:
		explicit CircuitBreakerException(const std::string &message)
			: Exception(message) {
		}
	};

	class CircuitBreakerOpenException final : public CircuitBreakerException {
	public:
		explicit CircuitBreakerOpenException(const std::string &message)
			: CircuitBreakerException("Circuit Breaker Open: " + message) {
		}
	};

	class CircuitBreakerTimeoutException final : public CircuitBreakerException {
	public:
		explicit CircuitBreakerTimeoutException(const std::string &message)
			: CircuitBreakerException("Circuit Breaker Timeout: " + message) {
		}
	};

	class CircuitBreakerCloseException final : public CircuitBreakerException {
	public:
		explicit CircuitBreakerCloseException(const std::string &message)
			: CircuitBreakerException("Circuit Breaker Close: " + message) {
		}
	};
}

#endif //NUANSA_UTILS_EXCEPTION_CIRCUIT_BREAKER_EXCEPTION_H
