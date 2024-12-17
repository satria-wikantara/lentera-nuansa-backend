#include "nuansa/services/token/token_repository.h"
#include "nuansa/database/db_connection_pool.h"
#include "nuansa/utils/log/log.h"

namespace nuansa::services::token {

    TokenRepository& TokenRepository::GetInstance() {
        static TokenRepository instance;
        return instance;
    }

    std::string FormatTimestamp(std::time_t time) {
        std::tm* tm = std::localtime(&time);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
        return std::string(buffer);
    }

    bool TokenRepository::SaveToken(const TokenService::Token& token,
                                  const std::string& userId,
                                  const std::string& tokenType) {
        try {
            auto conn = database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn{*conn};
            
            std::string expiryStr = FormatTimestamp(token.expiry);
            
            txn.exec_params(
                "INSERT INTO tokens (token_id, user_id, token_type, expiry, is_revoked) "
                "VALUES ($1, $2, $3, $4::timestamp, FALSE)",
                token.token_id,
                userId,
                tokenType,
                expiryStr
            );
            
            txn.commit();
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to save token: " << e.what();
            return false;
        }
    }

    bool TokenRepository::RevokeToken(const std::string& tokenId) {
        try {
            auto conn = database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn{*conn};
            
            auto result = txn.exec_params(
                "UPDATE tokens SET is_revoked = TRUE "
                "WHERE token_id = $1 AND NOT is_revoked",
                tokenId
            );
            
            txn.commit();
            return result.affected_rows() > 0;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to revoke token: " << e.what();
            return false;
        }
    }

    bool TokenRepository::IsTokenRevoked(const std::string& tokenId) const {
        try {
            auto conn = database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn{*conn};
            
            auto result = txn.exec_params(
                "SELECT is_revoked FROM tokens WHERE token_id = $1",
                tokenId
            );
            
            return !result.empty() && result[0][0].as<bool>();
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to check token revocation status: " << e.what();
            return true; // Assume revoked on error
        }
    }

    bool TokenRepository::IsTokenActive(const std::string& tokenId) const {
        try {
            auto conn = database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn{*conn};
            
            auto result = txn.exec_params(
                "SELECT COUNT(*) FROM tokens "
                "WHERE token_id = $1 AND NOT is_revoked AND expiry > CURRENT_TIMESTAMP",
                tokenId
            );
            
            return !result.empty() && result[0][0].as<int>() > 0;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to check token status: " << e.what();
            return false;
        }
    }

    bool TokenRepository::CleanupExpiredTokens() {
        try {
            auto conn = database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn{*conn};
            
            auto result = txn.exec(
                "UPDATE tokens SET is_revoked = TRUE "
                "WHERE NOT is_revoked AND expiry <= CURRENT_TIMESTAMP"
            );
            
            txn.commit();
            LOG_INFO << "Cleaned up " << result.affected_rows() << " expired tokens";
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to cleanup expired tokens: " << e.what();
            return false;
        }
    }

    std::optional<std::string> TokenRepository::GetUserIdFromToken(const std::string& tokenId) const {
        try {
            auto conn = database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn{*conn};
            
            auto result = txn.exec_params(
                "SELECT user_id FROM tokens "
                "WHERE token_id = $1 AND NOT is_revoked AND expiry > CURRENT_TIMESTAMP",
                tokenId
            );
            
            if (result.empty()) {
                return std::nullopt;
            }
            
            return result[0][0].as<std::string>();
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to get user ID from token: " << e.what();
            return std::nullopt;
        }
    }

    // Helper function to convert PostgreSQL timestamp to std::time_t
    std::time_t ConvertPgTimestamp(const std::string& timestamp) {
        std::tm tm = {};
        std::istringstream ss(timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        return std::mktime(&tm);
    }

    std::optional<std::time_t> TokenRepository::GetTokenExpiry(const std::string& tokenId) const {
        try {
            auto conn = database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn{*conn};
            
            auto result = txn.exec_params(
                "SELECT expiry::text FROM tokens WHERE token_id = $1",
                tokenId
            );
            
            if (result.empty()) {
                return std::nullopt;
            }
            
            return ConvertPgTimestamp(result[0][0].as<std::string>());
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to get token expiry: " << e.what();
            return std::nullopt;
        }
    }

    std::vector<TokenService::Token> TokenRepository::GetActiveTokensForUser(
        const std::string& userId) const {
        try {
            auto conn = database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn{*conn};
            
            auto result = txn.exec_params(
                "SELECT token_id, expiry::text, token_type "
                "FROM tokens "
                "WHERE user_id = $1 AND NOT is_revoked AND expiry > CURRENT_TIMESTAMP",
                userId
            );
            
            std::vector<TokenService::Token> tokens;
            for (const auto& row : result) {
                TokenService::Token token;
                token.token_id = row["token_id"].as<std::string>();
                token.expiry = ConvertPgTimestamp(row["expiry"].as<std::string>());
                tokens.push_back(token);
            }
            
            return tokens;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to get active tokens for user: " << e.what();
            return {};
        }
    }

    bool TokenRepository::RevokeAllTokensForUser(const std::string& userId) {
        try {
            auto conn = database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn{*conn};
            
            auto result = txn.exec_params(
                "UPDATE tokens SET is_revoked = TRUE "
                "WHERE user_id = $1 AND NOT is_revoked",
                userId
            );
            
            txn.commit();
            LOG_INFO << "Revoked " << result.affected_rows() << " tokens for user: " << userId;
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to revoke all tokens for user: " << e.what();
            return false;
        }
    }

    bool TokenRepository::IsTokenValid(const std::string& tokenId) const {
        return IsTokenActive(tokenId) && !IsTokenRevoked(tokenId);
    }

    std::optional<TokenService::Token> TokenRepository::GetToken(
        const std::string& tokenId) const {
        try {
            auto conn = database::ConnectionPool::GetInstance().AcquireConnection();
            pqxx::work txn{*conn};
            
            auto result = txn.exec_params(
                "SELECT token_id, expiry::text, user_id "
                "FROM tokens "
                "WHERE token_id = $1 AND NOT is_revoked",
                tokenId
            );
            
            if (result.empty()) {
                return std::nullopt;
            }
            
            TokenService::Token token;
            token.token_id = result[0]["token_id"].as<std::string>();
            token.expiry = ConvertPgTimestamp(result[0]["expiry"].as<std::string>());
            
            return token;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to get token: " << e.what();
            return std::nullopt;
        }
    }
} 