#ifndef NUANSA_SERVICES_TOKEN_TOKEN_REPOSITORY_H
#define NUANSA_SERVICES_TOKEN_TOKEN_REPOSITORY_H

#include "nuansa/utils/pch.h"
#include "nuansa/services/token/token_service.h"

namespace nuansa::services::token {
    class TokenRepository {
    public:
        static TokenRepository& GetInstance();
        
        bool SaveToken(const TokenService::Token& token, 
                      const std::string& userId,
                      const std::string& tokenType);
        
        bool RevokeToken(const std::string& tokenId);
        bool IsTokenRevoked(const std::string& tokenId) const;
        bool IsTokenActive(const std::string& tokenId) const;
        bool CleanupExpiredTokens();
        
        std::optional<std::string> GetUserIdFromToken(const std::string& tokenId) const;
        std::optional<std::time_t> GetTokenExpiry(const std::string& tokenId) const;
        
        std::vector<TokenService::Token> GetActiveTokensForUser(const std::string& userId) const;
        bool RevokeAllTokensForUser(const std::string& userId);
        
        bool IsTokenValid(const std::string& tokenId) const;
        std::optional<TokenService::Token> GetToken(const std::string& tokenId) const;
        
    private:
        TokenRepository() = default;
        
        bool ValidateTokenInDB(const std::string& tokenId) const;
        void CleanupOldTokens();

    };
}

#endif 