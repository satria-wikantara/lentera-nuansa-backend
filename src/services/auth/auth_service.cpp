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
#include "nuansa/config/config.h"

using namespace nuansa::config;

namespace nuansa::services::auth {
    AuthService &AuthService::GetInstance() {
        static AuthService instance;
        return instance;
    }

    AuthService::AuthService() 
        : gen(rd()), 
          httpClient(std::make_unique<utils::HttpClient>()) {
        // Get GitHub config from ServerConfig
        const auto& config = Config::GetInstance().GetServerConfig();
        GITHUB_CLIENT_ID = config.githubClientId;
        GITHUB_CLIENT_SECRET = config.githubClientSecret;
        GITHUB_REDIRECT_URI = config.githubRedirectUri;
        GITHUB_API_URL = config.githubApiUrl;
        GITHUB_TOKEN_VALIDATION_URL = config.githubTokenValidationUrl;
        GITHUB_USER_API_URL = config.githubUserApiUrl;
        GITHUB_USER_EMAILS_URL = config.githubUserEmailsUrl;

        GOOGLE_CLIENT_ID = config.googleClientId;
        GOOGLE_CLIENT_SECRET = config.googleClientSecret;
        GOOGLE_REDIRECT_URI = config.googleRedirectUri;
        GOOGLE_TOKEN_INFO_URL = config.googleTokenInfoUrl;
        GOOGLE_USER_INFO_URL = config.googleUserInfoUrl;

        if (GITHUB_CLIENT_ID.empty() || GITHUB_CLIENT_SECRET.empty() || GITHUB_REDIRECT_URI.empty()) {
            LOG_WARNING << "GitHub OAuth configuration is incomplete";
        }

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
        LOG_DEBUG << "Registering user with provider: " << static_cast<int>(request.GetAuthProvider());
        std::lock_guard<std::mutex> lock(authMutex);

        switch (request.GetAuthProvider()) {
            case AuthProvider::Custom:
                return HandleCustomRegistration(request);
            case AuthProvider::Google:
                LOG_DEBUG << "Registering user with provider: Google";
                return HandleOAuthRegistration(request);
            case AuthProvider::GitHub:
                LOG_DEBUG << "Registering user with provider: GitHub";
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
        LOG_DEBUG << "Handling OAuth registration";

        if (!request.GetOAuthCredentials()) {
            LOG_ERROR << "Missing OAuth credentials";
            return AuthResponse{false, "", "Missing OAuth credentials"};
        }

        std::optional<OAuthUserInfo> userInfo;

        LOG_DEBUG << "Validating OAuth token";
        
        // Validate OAuth token and get user info
        switch (request.GetAuthProvider()) {
            case AuthProvider::Google:
                LOG_DEBUG << "Validating Google OAuth token";
                userInfo = ValidateGoogleToken(*request.GetOAuthCredentials());
                break;
            case AuthProvider::GitHub:
                LOG_DEBUG << "Validating GitHub OAuth token";
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
            GOOGLE_TOKEN_INFO_URL + "?access_token=" + credentials.accessToken;

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
            auto userResponse = httpClient->Get(GOOGLE_USER_INFO_URL, headers);
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
        LOG_DEBUG << "Validating GitHub OAuth token";
        try {
            if (!credentials.code || !credentials.redirectUri) {
                LOG_ERROR << "Missing required OAuth code or redirect URI";
                return std::nullopt;
            }

            // Exchange code for access token
            nlohmann::json requestBody = {
                {"client_id", GITHUB_CLIENT_ID},
                {"client_secret", GITHUB_CLIENT_SECRET},
                {"code", *credentials.code},
                {"redirect_uri", GITHUB_REDIRECT_URI}
            };

            LOG_DEBUG << "Exchanging code for token";
            auto tokenResponse = httpClient->Post(
                "https://github.com/login/oauth/access_token",
                requestBody.dump(),
                {
                    "Accept: application/json",
                    "Content-Type: application/json",
                    "User-Agent: Nuansa-App"
                }
            );

            if (!tokenResponse.success) {
                LOG_ERROR << "Failed to exchange code for token: " << tokenResponse.error 
                         << " Response: " << tokenResponse.body;
                return std::nullopt;
            }

            // Parse the token response
            auto tokenJson = nlohmann::json::parse(tokenResponse.body);
            std::string accessToken = tokenJson["access_token"].get<std::string>();
            
            LOG_DEBUG << "Successfully obtained access token";

            // Now get user info using the access token
            auto userResponse = httpClient->Get(
                "https://api.github.com/user",
                {
                    "Authorization: Bearer " + accessToken,  // Changed to Bearer as per docs
                    "Accept: application/vnd.github+json",
                    "X-GitHub-Api-Version: 2022-11-28",
                    "User-Agent: Nuansa-App"
                }
            );

            if (!userResponse.success) {
                LOG_ERROR << "Failed to get user info: " << userResponse.error;
                return std::nullopt;
            }

            auto userInfo = nlohmann::json::parse(userResponse.body);
            
            // Get user email
            auto emailResponse = httpClient->Get(
                GITHUB_USER_EMAILS_URL,
                {
                    "Authorization: Bearer " + accessToken,
                    "Accept: application/vnd.github+json",
                    "X-GitHub-Api-Version: 2022-11-28",
                    "User-Agent: Nuansa-App"
                }
            );

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
                LOG_ERROR << "Could not find verified primary email for GitHub user";
                return std::nullopt;
            }

            return OAuthUserInfo{
                std::to_string(userInfo["id"].get<int>()),
                primaryEmail,
                userInfo["login"].get<std::string>(),
                userInfo["avatar_url"].get<std::string>()
            };

        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to validate GitHub token: " << e.what();
            return std::nullopt;
        }
    }

    bool AuthService::VerifyGitHubScopes(const std::vector<std::string>& headers) {
        // Look for the X-OAuth-Scopes header
        for (const auto& header : headers) {
            if (header.find("X-OAuth-Scopes: ") == 0) {
                std::string scopes = header.substr(15); // Length of "X-OAuth-Scopes: "
                return (scopes.find("user") != std::string::npos || 
                       scopes.find("read:user") != std::string::npos) &&
                       scopes.find("user:email") != std::string::npos;
            }
        }
        return false;
    }
}
