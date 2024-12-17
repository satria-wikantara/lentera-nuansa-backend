//
// Created by I Gede Panca Sutresna on 17/12/24.
//
#ifndef NUANSA_SERVICES_TOKEN_TOKEN_SERVICE_H
#define NUANSA_SERVICES_TOKEN_TOKEN_SERVICE_H

#include "nuansa/utils/pch.h"

namespace nuansa::services::token {
    
    class TokenService {
    public:
        struct Token {
            std::string token_id;
            std::time_t expiry;
            std::string signature;
            std::string refresh_token;
            
            nlohmann::json ToJson() const {
                return {
                    {"token_id", token_id},
                    {"expiry", expiry},
                    {"signature", signature}
                };
            }
        };

        explicit TokenService(const std::string& secret_key);
        
        // Token generation methods
        Token GenerateAccessToken(const std::string& user_id, int expiry_minutes = 60);
        Token GenerateRefreshToken(const std::string& user_id, int expiry_days = 30);
        
        // Token validation methods
        bool VerifyToken(const Token& token) const;
        bool IsTokenExpired(const Token& token) const;
        std::optional<std::string> ExtractUserIdFromToken(const Token& token) const;
        
        // Token revocation
        void RevokeToken(const std::string& token_id);
        bool IsTokenRevoked(const std::string& token_id) const;

        // Token validation and management
        bool ValidateToken(const std::string& token_id) const;
        std::optional<std::string> GetUserIdFromToken(const std::string& token_id) const;
        void CleanupExpiredTokens();
        
        // Batch operations
        std::vector<Token> GetActiveTokensForUser(const std::string& user_id) const;
        void RevokeAllTokensForUser(const std::string& user_id);

        // User token operations
        std::optional<std::string> GetUsernameFromToken(const std::string& token_id) const;
        bool ValidateAndGetUsername(const std::string& token_id, std::string& username) const;
        
        // Token lifecycle
        void LogoutToken(const std::string& token_id);
        bool SaveNewToken(const Token& token, const std::string& username, const std::string& type);

    private:
        const std::string secret_key_;
        std::random_device rd_;
        std::mt19937 gen_;
        mutable std::mutex mutex_;
        std::unordered_set<std::string> revoked_tokens_;

        std::string CreateSignature(const std::string& data) const;
        std::string CreateTokenData(const std::string& token_id, 
                                  std::time_t expiry, 
                                  const std::string& user_id) const;

        // Helper methods
        bool ValidateSignature(const Token& token) const;
        bool ValidateExpiry(const Token& token) const;
    };
}

#endif // NUANSA_SERVICES_TOKEN_TOKEN_SERVICE_H