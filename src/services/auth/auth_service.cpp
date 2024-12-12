//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/utils/pch.h"

#include "nuansa/services/auth/auth_service.h"
#include "nuansa/messages/message_types.h"
#include "nuansa/services/user/user_service.h"
#include "nuansa/utils/validation.h"
#include "nuansa/services/auth/auth_message.h"
#include "nuansa/services/auth/register_message.h"
#include "nuansa/utils/http_client.h"

namespace nuansa::services::auth {
    AuthService &AuthService::GetInstance() {
        static AuthService instance;
        return instance;
    }

    AuthService::AuthService() 
        : gen(rd()), 
          httpClient(std::make_unique<utils::HttpClient>()) {
        // TODO: In production, load from secure database
        // For demonstration, adding some test users with hashed passwords
        userCredentials["alice"] = crypt("password123", "$6$random_salt");
        userCredentials["bob"] = crypt("password456", "$6$random_salt");
    }

    nuansa::services::auth::AuthResponse AuthService::Authenticate(const nuansa::services::auth::AuthRequest &request) {
        std::lock_guard<std::mutex> lock(authMutex);

        // Delegate authentication to UserService
        if (auto &userService = nuansa::services::user::UserService::GetInstance(); !userService.AuthenticateUser(
            request.GetUsername(), request.GetPassword())) {
            return nuansa::services::auth::AuthResponse{false, "", "Invalid credentials"};
        }

        // Generate new token
        const std::string token = GenerateToken(request.GetUsername());

        // Store token info
        const TokenInfo tokenInfo{
            request.GetUsername(),
            std::chrono::system_clock::now() + std::chrono::hours(24)
        };
        activeTokens[token] = tokenInfo;

        return nuansa::services::auth::AuthResponse{true, token, "Authentication successful"};
    }

    bool AuthService::ValidateToken(const std::string &token) {
        std::lock_guard<std::mutex> lock(authMutex);

        const auto it = activeTokens.find(token);
        if (it == activeTokens.end()) {
            return false;
        }

        if (std::chrono::system_clock::now() > it->second.expirationTime) {
            activeTokens.erase(it);
            return false;
        }

        return true;
    }

    void AuthService::Logout(const std::string &token) {
        std::lock_guard<std::mutex> lock(authMutex);
        activeTokens.erase(token);
    }

    std::optional<std::string> AuthService::GetUsernameFromToken(const std::string &token) {
        std::lock_guard<std::mutex> lock(authMutex);

        if (const auto it = activeTokens.find(token); it != activeTokens.end() &&
                                                      std::chrono::system_clock::now() <= it->second.expirationTime) {
            return it->second.username;
        }

        return std::nullopt;
    }

    std::string AuthService::GenerateToken(const std::string &username) {
        boost::uuids::random_generator gen;
        const auto uuid = gen();
        return boost::uuids::to_string(uuid);
    }

    void AuthService::CleanupExpiredTokens() {
        std::lock_guard<std::mutex> lock(authMutex);
        const auto now = std::chrono::system_clock::now();

        for (auto it = activeTokens.begin(); it != activeTokens.end();) {
            if (now > it->second.expirationTime) {
                it = activeTokens.erase(it);
            } else {
                ++it;
            }
        }
    }

    nuansa::services::auth::AuthResponse AuthService::Register(const nuansa::services::auth::RegisterRequest &request) {
        std::lock_guard<std::mutex> lock(authMutex);

        switch (request.GetAuthProvider()) {
            case AuthProvider::Custom:
                return HandleCustomRegistration(request);
            case AuthProvider::Google:
            case AuthProvider::GitHub:
                return HandleOAuthRegistration(request);
            default:
                return AuthResponse{false, "", "Unsupported authentication provider"};
        }
    }

    AuthResponse AuthService::HandleCustomRegistration(const RegisterRequest& request) {
        if (!request.GetUsername() || !request.GetEmail() || !request.GetPassword()) {
            return AuthResponse{false, "", "Missing required fields for custom registration"};
        }

        // Validate username
        if (!nuansa::utils::Validation::ValidateUsername(*request.GetUsername())) {
            LOG_WARNING << "Invalid username format: " << *request.GetUsername();
            return AuthResponse{false, "", "Invalid username format"};
        }

        // Validate email
        if (!nuansa::utils::Validation::ValidateEmail(*request.GetEmail())) {
            LOG_WARNING << "Invalid email format: " << *request.GetEmail();
            return AuthResponse{false, "", "Invalid email format"};
        }

        // Validate password
        if (!nuansa::utils::Validation::ValidatePassword(*request.GetPassword())) {
            LOG_WARNING << "Invalid password format";
            return AuthResponse{false, "", "Invalid password format"};
        }

        // Create user with custom credentials
        auto& userService = nuansa::services::user::UserService::GetInstance();
        const std::string salt = nuansa::models::User::GenerateSalt();
        const std::string hashedPassword = nuansa::utils::crypto::CryptoUtil::HashPassword(
            *request.GetPassword(), salt);

        if (!userService.CreateUser(nuansa::models::User{
            *request.GetUsername(), 
            *request.GetEmail(), 
            hashedPassword, 
            salt,
            ""  // Empty picture for custom registration
        })) {
            return AuthResponse{false, "", "Registration failed"};
        }

        // Generate token and complete registration
        const std::string token = GenerateToken(*request.GetUsername());
        activeTokens[token] = TokenInfo{
            *request.GetUsername(),
            std::chrono::system_clock::now() + std::chrono::hours(24)
        };

        return AuthResponse{true, token, "Registration successful"};
    }

    AuthResponse AuthService::HandleOAuthRegistration(const RegisterRequest& request) {
        if (!request.GetOAuthCredentials()) {
            return AuthResponse{false, "", "Missing OAuth credentials"};
        }

        std::optional<OAuthUserInfo> userInfo;
        
        // Validate OAuth token and get user info
        switch (request.GetAuthProvider()) {
            case AuthProvider::Google:
                userInfo = ValidateGoogleToken(*request.GetOAuthCredentials());
                break;
            case AuthProvider::GitHub:
                userInfo = ValidateGitHubToken(*request.GetOAuthCredentials());
                break;
            default:
                return AuthResponse{false, "", "Unsupported OAuth provider"};
        }

        if (!userInfo) {
            return AuthResponse{false, "", "Failed to validate OAuth token"};
        }

        // Check if user exists, if not create new user
        auto& userService = nuansa::services::user::UserService::GetInstance();
        if (!userService.UserExists(userInfo->email)) {
            // Create user with OAuth information
            if (!userService.CreateUser(nuansa::models::User{
                userInfo->username,
                userInfo->email,
                "",  // No password for OAuth users
                "",  // No salt needed
                userInfo->picture
            })) {
                return AuthResponse{false, "", "Failed to create user account"};
            }
        }

        // Generate token and complete registration
        const std::string token = GenerateToken(userInfo->email);
        activeTokens[token] = TokenInfo{
            userInfo->email,
            std::chrono::system_clock::now() + std::chrono::hours(24)
        };

        return AuthResponse{true, token, "OAuth registration successful"};
    }

    std::optional<AuthService::OAuthUserInfo> AuthService::ValidateGoogleToken(
        const OAuthCredentials& credentials) {
        // Set up request headers with required fields
        std::vector<std::string> headers = {
            "Authorization: Bearer " + credentials.accessToken,
            "Content-Type: application/json"
        };

        // Google's OAuth2 v3 tokeninfo endpoint
        const std::string tokenInfoUrl = 
            GOOGLE_OAUTH_TOKENINFO_URL + "?access_token=" + credentials.accessToken;

        // First validate the token
        auto tokenResponse = httpClient->Get(tokenInfoUrl);
        if (!tokenResponse.success) {
            LOG_ERROR << "Failed to validate Google token: " << tokenResponse.error;
            return std::nullopt;
        }

        try {
            auto tokenInfo = nlohmann::json::parse(tokenResponse.body);
            
            // Validate token claims
            if (!ValidateGoogleTokenClaims(tokenInfo)) {
                LOG_ERROR << "Invalid token claims";
                return std::nullopt;
            }

            // If token is valid, get user info
            auto userResponse = httpClient->Get(GOOGLE_OAUTH_USERINFO_URL, headers);
            if (!userResponse.success) {
                LOG_ERROR << "Failed to get Google user info: " << userResponse.error;
                return std::nullopt;
            }

            auto userInfo = nlohmann::json::parse(userResponse.body);
            return OAuthUserInfo{
                userInfo["sub"].get<std::string>(),
                userInfo["email"].get<std::string>(),
                userInfo["name"].get<std::string>(),
                userInfo["picture"].get<std::string>()
            };

        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to parse Google response: " << e.what();
            return std::nullopt;
        }
    }

    bool AuthService::ValidateGoogleTokenClaims(const nlohmann::json& tokenInfo) {
        try {
            // Verify required fields
            if (!tokenInfo.contains("aud") || !tokenInfo.contains("exp")) {
                return false;
            }

            // Verify client ID (aud claim)
            if (tokenInfo["aud"] != GOOGLE_CLIENT_ID) {
                LOG_ERROR << "Token was not issued for this application";
                return false;
            }

            // Verify expiration
            const auto exp = tokenInfo["exp"].get<int64_t>();
            const auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000;
            if (now >= exp) {
                LOG_ERROR << "Token has expired";
                return false;
            }

            // Optionally verify other claims (issuer, email verified, etc.)
            if (tokenInfo.contains("iss") && 
                tokenInfo["iss"] != "https://accounts.google.com" && 
                tokenInfo["iss"] != "accounts.google.com") {
                LOG_ERROR << "Invalid token issuer";
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            LOG_ERROR << "Error validating token claims: " << e.what();
            return false;
        }
    }

    std::optional<AuthService::OAuthUserInfo> AuthService::ValidateGitHubToken(
        const OAuthCredentials& credentials) {
        // Set up request headers with required fields
        std::vector<std::string> headers = {
            "Authorization: Bearer " + credentials.accessToken,
            "Accept: application/vnd.github.v3+json",
            "User-Agent: Nuansa-App"  // GitHub requires a User-Agent header
        };

        // First validate the token using GitHub's token validation endpoint
        auto tokenResponse = httpClient->Get(GITHUB_TOKEN_VALIDATION_URL, headers);
        if (!tokenResponse.success) {
            LOG_ERROR << "Failed to validate GitHub token: " << tokenResponse.error;
            return std::nullopt;
        }

        try {
            auto tokenInfo = nlohmann::json::parse(tokenResponse.body);
            
            // Validate token scopes and other properties
            if (!ValidateGitHubTokenClaims(tokenInfo)) {
                LOG_ERROR << "Invalid GitHub token claims";
                return std::nullopt;
            }

            // If token is valid, get user info
            auto userResponse = httpClient->Get(GITHUB_USER_API_URL, headers);
            if (!userResponse.success) {
                LOG_ERROR << "Failed to get GitHub user info: " << userResponse.error;
                return std::nullopt;
            }

            auto userInfo = nlohmann::json::parse(userResponse.body);
            
            // Get user email (might be private, need separate call)
            auto emailResponse = httpClient->Get(GITHUB_USER_EMAILS_URL, headers);
            std::string primaryEmail;
            if (emailResponse.success) {
                auto emails = nlohmann::json::parse(emailResponse.body);
                for (const auto& email : emails) {
                    if (email["primary"].get<bool>() && email["verified"].get<bool>()) {
                        primaryEmail = email["email"].get<std::string>();
                        break;
                    }
                }
            }

            if (primaryEmail.empty() && userInfo.contains("email")) {
                primaryEmail = userInfo["email"].get<std::string>();
            }

            if (primaryEmail.empty()) {
                LOG_ERROR << "Could not find primary email for GitHub user";
                return std::nullopt;
            }

            return OAuthUserInfo{
                std::to_string(userInfo["id"].get<int>()),
                primaryEmail,
                userInfo["login"].get<std::string>(),
                userInfo["avatar_url"].get<std::string>()
            };

        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to parse GitHub response: " << e.what();
            return std::nullopt;
        }
    }

    bool AuthService::ValidateGitHubTokenClaims(const nlohmann::json& tokenInfo) {
        try {
            // Check if token is valid
            if (!tokenInfo.contains("active") || !tokenInfo["active"].get<bool>()) {
                LOG_ERROR << "GitHub token is inactive";
                return false;
            }

            // Verify required scopes
            if (tokenInfo.contains("scope")) {
                std::string scopes = tokenInfo["scope"].get<std::string>();
                if (scopes.find("read:user") == std::string::npos || 
                    scopes.find("user:email") == std::string::npos) {
                    LOG_ERROR << "Token missing required scopes";
                    return false;
                }
            }

            // Verify app client_id if available
            if (tokenInfo.contains("client_id") && 
                tokenInfo["client_id"].get<std::string>() != GITHUB_CLIENT_ID) {
                LOG_ERROR << "Token was not issued for this application";
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            LOG_ERROR << "Error validating GitHub token claims: " << e.what();
            return false;
        }
    }
}
