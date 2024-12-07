//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_ERRORS_DATABASE_ERROR_H
#define NUANSA_UTILS_ERRORS_DATABASE_ERROR_H

#include <string>

#include "nuansa/pch/pch.h"
#include "nuansa/utils/errors/exception.h"

namespace nuansa::utils::errors {
	class DatabaseException : public nuansa::utils::errors::Exception {
	public:
		explicit DatabaseException(const std::string &message)
			: Exception(message) {
		}
	};

	class DatabaseCreateConnectionException : public DatabaseException {
	public:
		explicit DatabaseCreateConnectionException(const std::string &message)
			: DatabaseException("Database connection creation error: " + message) {
		}
	};

	class DatabaseConnectionPoolInitializationException : public DatabaseException {
	public:
		explicit DatabaseConnectionPoolInitializationException(const std::string &message)
			: DatabaseException("Database connection pool initialization error: " + message) {
		}
	};

	class DatabaseQueryExecutionException : public DatabaseException {
	public:
		explicit DatabaseQueryExecutionException(const std::string &message)
			: DatabaseException("Database query execution error: " + message) {
		}
	};

	class DatabaseConnectionPoolException : public DatabaseException {
	public:
		explicit DatabaseConnectionPoolException(const std::string &message)
			: DatabaseException("Database connection pool error: " + message) {
		}
	};

	class DatabaseConnectionBrokenException final : public pqxx::broken_connection {
	public:
		explicit DatabaseConnectionBrokenException(const std::string &message)
			: pqxx::broken_connection(message) {
		}
	};
}

#endif //NUANSA_UTILS_ERRORS_DATABASE_ERROR_H
