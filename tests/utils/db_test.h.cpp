//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#ifndef DB_TEST_H
#define DB_TEST_H

#include "nuansa/pch/pch.h"
#include "nuansa/config/config.h"
#include "nuansa/database/db_connection_pool.h"

namespace nuansa::tests::utils {
    inline void InitTestDatabase() {
        try {
            auto conn = nuansa::database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn(*conn);

            // Create tables needed for tests
            txn.exec(R"(
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    username VARCHAR(255) UNIQUE NOT NULL,
                    email VARCHAR(255) UNIQUE NOT NULL,
                    password_hash VARCHAR(255) NOT NULL
                    -- add other fields as needed
                )
            )");

            txn.commit();
        } catch (const std::exception &e) {
            std::cerr << "Failed to initialize test database: " << e.what() << std::endl;
            throw;
        }
    }
}

#endif // DB_TEST_H
