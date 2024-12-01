//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_ERRORS_DATABASE_ERROR_H
#define NUANSA_UTILS_ERRORS_DATABASE_ERROR_H

#include <string>

#include "nuansa/pch/pch.h"

namespace nuansa::utils::error {
	class DatabaseError : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	class DatabaseCreateConnectionError : public DatabaseError {
	public:
		explicit DatabaseCreateConnectionError(const std::string &message)
			: DatabaseError("Database connection creation error: " + message) {
		}
	};

	class DatabaseConnectionPoolInitializationError : public DatabaseError {
	public:
		explicit DatabaseConnectionPoolInitializationError(const std::string &message)
			: DatabaseError("Database connection pool initialization error: " + message) {
		}
	};

	class DatabaseQueryExecutionError : public DatabaseError {
	public:
		explicit DatabaseQueryExecutionError(const std::string &message)
			: DatabaseError("Database query execution error: " + message) {
		}
	};

	class DatabaseConnectionPoolError : public DatabaseError {
	public:
		explicit DatabaseConnectionPoolError(const std::string &message)
			: DatabaseError("Database connection pool error: " + message) {
		}
	};
}

#endif //NUANSA_UTILS_ERRORS_DATABASE_ERROR_H
