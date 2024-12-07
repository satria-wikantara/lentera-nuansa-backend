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
			: DatabaseException("Database connection creation exception: " + message) {
		}
	};

	class DatabaseConnectionPoolInitializationException : public DatabaseException {
	public:
		explicit DatabaseConnectionPoolInitializationException(const std::string &message)
			: DatabaseException("Database connection pool initialization exception: " + message) {
		}
	};

	class DatabaseQueryExecutionException : public DatabaseException {
	public:
		explicit DatabaseQueryExecutionException(const std::string &message)
			: DatabaseException("Database query execution exception: " + message) {
		}
	};

	class DatabaseConnectionPoolException : public DatabaseException {
	public:
		explicit DatabaseConnectionPoolException(const std::string &message)
			: DatabaseException("Database connection pool exception: " + message) {
		}
	};

	class DatabaseBrokenConnectionException final : public DatabaseException {
	public:
		explicit DatabaseBrokenConnectionException(const std::string &message)
			: DatabaseException("Database broken connection exception: " + message) {
		}
	};

	class NonTransientDatabaseException final : public DatabaseException {
	public:
		explicit NonTransientDatabaseException(const std::string &message)
			: DatabaseException("Non-Transient database exception: " + message) {
		}
	};
}

#endif //NUANSA_UTILS_ERRORS_DATABASE_ERROR_H
