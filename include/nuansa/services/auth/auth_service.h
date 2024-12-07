#ifndef NUANSA_SERVICES_AUTH_AUTH_SERVICE_H
#define NUANSA_SERVICES_AUTH_AUTH_SERVICE_H

#include "nuansa/utils/pch.h"

#include "nuansa/services/auth/auth_message.h"
#include "nuansa/services/auth/register_message.h"

namespace nuansa::services::auth {
	class AuthService {
	public:
		AuthService();

		static AuthService &GetInstance();

		nuansa::services::auth::AuthResponse Authenticate(const nuansa::services::auth::AuthRequest &request);

		bool ValidateToken(const std::string &token);

		void Logout(const std::string &token);

		std::optional<std::string> GetUsernameFromToken(const std::string &token);

		nuansa::services::auth::AuthResponse Register(const nuansa::services::auth::RegisterRequest &request);

	private:
		static std::string GenerateToken(const std::string &username);

		void CleanupExpiredTokens();

		struct TokenInfo {
			std::string username;
			std::chrono::system_clock::time_point expirationTime;
		};

		std::unordered_map<std::string, std::string> userCredentials; // username -> password hash
		std::unordered_map<std::string, TokenInfo> activeTokens; // token -> TokenInfo
		std::mutex authMutex;
		std::random_device rd;
		std::mt19937 gen;
	};
}

#endif /* NUANSA_SERVICES_AUTH_AUTH_SERVICE_H */
