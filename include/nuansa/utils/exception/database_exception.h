//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_EXCEPTION_DATABASE_EXCEPTION_H
#define NUANSA_UTILS_EXCEPTION_DATABASE_EXCEPTION_H

#include <string>

#include "nuansa/utils/exception/exception.h"

namespace nuansa::utils::exception {
	class DatabaseException : public nuansa::utils::exception::Exception {
	public:
		explicit DatabaseException(const std::string &message)
			: Exception(message) {
		}
	};

	class DatabaseCreateConnectionException final : public DatabaseException {
	public:
		explicit DatabaseCreateConnectionException(const std::string &message)
			: DatabaseException("Database connection creation exception: " + message) {
		}
	};

	class DatabaseConnectionPoolInitializationException final : public DatabaseException {
	public:
		explicit DatabaseConnectionPoolInitializationException(const std::string &message)
			: DatabaseException("Database connection pool initialization exception: " + message) {
		}
	};

	class DatabaseQueryExecutionException final : public DatabaseException {
	public:
		explicit DatabaseQueryExecutionException(const std::string &message)
			: DatabaseException("Database query execution exception: " + message) {
		}
	};

	class DatabaseConnectionPoolException final : public DatabaseException {
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

#endif //NUANSA_UTILS_EXCEPTION_DATABASE_EXCEPTION_H
