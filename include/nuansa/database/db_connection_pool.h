//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_DATABASE_DB_CONNECTION_POOL_H
#define NUANSA_DATABASE_DB_CONNECTION_POOL_H

#include "nuansa/pch/pch.h"
#include "nuansa/utils/patterns/circuit_breaker.h"

namespace nuansa::database {
	class ConnectionPool {
	public:
	private:
		ConnectionPool() = default;

		~ConnectionPool();
		ConnectionPool(const ConnectionPool&) = delete;
	};
};

#endif //NUANSA_DATABASE_DB_CONNECTION_POOL_H
