#include "nuansa/utils/random_generator.h"
#include "nuansa/services/token/token_service.h"
#include "nuansa/services/token/token_repository.h"
#include "nuansa/database/db_connection_pool.h"

namespace nuansa::services::token {

    TokenService::TokenService(const std::string& secret_key)
        : secret_key_(secret_key), gen_(rd_()) {
        if (secret_key_.empty()) {
            throw std::runtime_error("Secret key cannot be empty");
        }
    }

    std::string TokenService::CreateSignature(const std::string& data) const {
        unsigned char hash[EVP_MAX_MD_SIZE];
        size_t hashLen;

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        EVP_PKEY* pkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr,
            reinterpret_cast<const unsigned char*>(secret_key_.c_str()),
            secret_key_.length());

        if (!ctx || !pkey) {
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Failed to create EVP context");
        }

        if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) != 1 ||
            EVP_DigestSignUpdate(ctx, data.c_str(), data.length()) != 1 ||
            EVP_DigestSignFinal(ctx, hash, &hashLen) != 1) {
            EVP_MD_CTX_free(ctx);
            EVP_PKEY_free(pkey);
            throw std::runtime_error("Signature operation failed");
        }

        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);

        char hex[EVP_MAX_MD_SIZE * 2 + 1];
        for (size_t i = 0; i < hashLen; i++) {
            snprintf(hex + i * 2, 3, "%02x", hash[i]);
        }
        return std::string(hex);
    }



    std::string TokenService::CreateTokenData(const std::string& token_id, 
                                            std::time_t expiry,
                                            const std::string& user_id) const {
        return token_id + ":" + std::to_string(expiry) + ":" + user_id;
    }

    TokenService::Token TokenService::GenerateAccessToken(const std::string& user_id, int expiry_minutes) {
        Token token;
        token.token_id = utils::RandomGenerator::GenerateString(32);  // Will generate UUID
        token.expiry = std::time(nullptr) + (expiry_minutes * 60);
        
        std::string data = CreateTokenData(token.token_id, token.expiry, user_id);
        token.signature = CreateSignature(data);
        
        return token;
    }

    TokenService::Token TokenService::GenerateRefreshToken(const std::string& user_id, int expiry_days) {
        Token token;
        token.token_id = utils::RandomGenerator::GenerateString(64);  // Longer for refresh tokens
        token.expiry = std::time(nullptr) + (expiry_days * 24 * 60 * 60);
        
        std::string data = CreateTokenData(token.token_id, token.expiry, user_id);
        token.signature = CreateSignature(data);
        
        return token;
    }

    bool TokenService::VerifyToken(const Token& token) const {
        if (!ValidateExpiry(token) || !ValidateSignature(token)) {
            return false;
        }
        
        auto& repo = services::token::TokenRepository::GetInstance();
        return !repo.IsTokenRevoked(token.token_id);
    }

    bool TokenService::IsTokenExpired(const Token& token) const {
        return std::time(nullptr) > token.expiry;
    }

    void TokenService::RevokeToken(const std::string& token_id) {
        services::token::TokenRepository::GetInstance().RevokeToken(token_id);
    }

    bool TokenService::IsTokenRevoked(const std::string& token_id) const {
        return services::token::TokenRepository::GetInstance().IsTokenRevoked(token_id);
    }

    std::optional<std::string> TokenService::ExtractUserIdFromToken(const Token& token) const {
        if (!VerifyToken(token)) {
            return std::nullopt;
        }

        try {
            // Parse token data to extract user_id
            std::string data = CreateTokenData(token.token_id, token.expiry, "");
            auto pos = data.find_last_of(':');
            if (pos != std::string::npos) {
                return data.substr(pos + 1);
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to extract user ID from token: " << e.what();
        }
        
        return std::nullopt;
    }

    bool TokenService::ValidateToken(const std::string& token_id) const {
        auto& repo = services::token::TokenRepository::GetInstance();
        
        if (!repo.IsTokenActive(token_id)) {
            return false;
        }

        auto expiry = repo.GetTokenExpiry(token_id);
        if (!expiry || std::time(nullptr) > *expiry) {
            repo.RevokeToken(token_id);
            return false;
        }

        return true;
    }

    void TokenService::CleanupExpiredTokens() {
        services::token::TokenRepository::GetInstance().CleanupExpiredTokens();
    }

    void TokenService::LogoutToken(const std::string& token_id) {
        auto& repo = services::token::TokenRepository::GetInstance();
        
        if (repo.IsTokenActive(token_id)) {
            if (repo.RevokeToken(token_id)) {
                LOG_INFO << "Token revoked successfully: " << token_id;
            } else {
                LOG_ERROR << "Failed to revoke token: " << token_id;
            }
        } else {
            LOG_WARNING << "Attempted to revoke inactive or non-existent token: " << token_id;
        }
    }

    std::optional<std::string> TokenService::GetUsernameFromToken(const std::string& token_id) const {
        auto& repo = services::token::TokenRepository::GetInstance();
        
        if (!repo.IsTokenActive(token_id)) {
            return std::nullopt;
        }

        return repo.GetUserIdFromToken(token_id);
    }

    bool TokenService::SaveNewToken(const Token& token, const std::string& username, const std::string& type) {
        auto& repo = services::token::TokenRepository::GetInstance();
        
        if (!repo.SaveToken(token, username, type)) {
            LOG_ERROR << "Failed to save token for user: " << username;
            return false;
        }
        
        return true;
    }

    bool TokenService::ValidateAndGetUsername(const std::string& token_id, std::string& username) const {
        if (!ValidateToken(token_id)) {
            return false;
        }
        
        auto user = GetUsernameFromToken(token_id);
        if (!user) {
            return false;
        }
        
        username = *user;
        return true;
    }

    std::vector<TokenService::Token> TokenService::GetActiveTokensForUser(const std::string& user_id) const {
        auto& repo = services::token::TokenRepository::GetInstance();
        std::vector<Token> tokens;
        
        try {
            // Implementation will depend on TokenRepository's interface
            // This is a placeholder for the actual implementation
            tokens = repo.GetActiveTokensForUser(user_id);
            LOG_INFO << "Retrieved active tokens for user: " << user_id;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to get active tokens for user: " << user_id << ": " << e.what();
        }
        
        return tokens;
    }

    void TokenService::RevokeAllTokensForUser(const std::string& user_id) {
        auto& repo = services::token::TokenRepository::GetInstance();
        try {
            repo.RevokeAllTokensForUser(user_id);
            LOG_INFO << "Revoked all tokens for user: " << user_id;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to revoke tokens for user: " << user_id << ": " << e.what();
        }
    }

    bool TokenService::ValidateSignature(const Token& token) const {
        std::string data = CreateTokenData(token.token_id, token.expiry, "");
        std::string expected_signature = CreateSignature(data);
        return token.signature == expected_signature;
    }

    bool TokenService::ValidateExpiry(const Token& token) const {
        return std::time(nullptr) <= token.expiry;
    }
}