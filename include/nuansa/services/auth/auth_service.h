#ifndef NUANSA_SERVICES_AUTH_AUTH_SERVICE_H
#define NUANSA_SERVICES_AUTH_AUTH_SERVICE_H

#include "nuansa/utils/pch.h"

#include "nuansa/services/auth/auth_message.h"
#include "nuansa/services/auth/register_message.h"
#include "nuansa/services/auth/auth_types.h"
#include "nuansa/utils/http_client.h"

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

		
		// OAuth user information structure
		struct OAuthUserInfo {
			std::string id;        // Unique ID from provider
			std::string email;     // User's email
			std::string username;  // User's name/username
			std::string picture;   // Profile picture URL
		};

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

		bool ValidateGoogleTokenClaims(const nlohmann::json& tokenInfo);
		bool ValidateGitHubTokenClaims(const nlohmann::json& tokenInfo);
		bool VerifyGitHubScopes(const std::vector<std::string>& headers);

		// OAuth methods
		AuthResponse HandleOAuthRegistration(const RegisterRequest& request);
		AuthResponse HandleCustomRegistration(const RegisterRequest& request);
		
		// OAuth provider-specific methods
		std::optional<OAuthUserInfo> ValidateGoogleToken(const OAuthCredentials& credentials);
		std::optional<OAuthUserInfo> ValidateGitHubToken(const OAuthCredentials& credentials);
		
		// Google
		std::string GOOGLE_CLIENT_ID;
		std::string GOOGLE_CLIENT_SECRET;
		std::string GOOGLE_REDIRECT_URI;
		std::string GOOGLE_TOKEN_INFO_URL;
		std::string GOOGLE_USER_INFO_URL;
		
		// Github
		std::string GITHUB_CLIENT_ID;
		std::string GITHUB_CLIENT_SECRET;
		std::string GITHUB_REDIRECT_URI;
		
		// Github endpoints (these can stay const)
		std::string GITHUB_API_URL;
		std::string GITHUB_TOKEN_VALIDATION_URL;
		std::string GITHUB_USER_API_URL;
		std::string GITHUB_USER_EMAILS_URL;

		std::unique_ptr<utils::HttpClient> httpClient;

		
	};
}

#endif /* NUANSA_SERVICES_AUTH_AUTH_SERVICE_H */
