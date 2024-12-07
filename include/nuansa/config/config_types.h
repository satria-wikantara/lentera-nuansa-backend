//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_CONFIG_CONFIG_TYPES_H
#define NUANSA_CONFIG_CONFIG_TYPES_H

#include <cstdint>

namespace nuansa::config {
	struct ServerConfig {
		uint16_t port;
		std::string host;
		std::string logLevel;
		std::string logPath;
	};

	struct DatabaseConfig {
		std::string host;
		uint16_t port;
		std::string database_name;
		std::string username;
		std::string password;
		size_t pool_size;
		std::string connection_string;
	};

	struct CircuitBreakerConfig {
		size_t failureThreshold{5};
		size_t successThreshold{2};
		uint64_t resetTimeoutSeconds{60};
		uint64_t timeoutSeconds{2};
	};

	// Main configuration structure
	struct ApplicationConfig {
		ServerConfig server;
		DatabaseConfig database;
		CircuitBreakerConfig circuitBreaker;
	};
}

#endif //NUANSA_CONFIG_CONFIG_TYPES_H
