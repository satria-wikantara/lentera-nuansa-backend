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
#include "nuansa/services/token/token_service.h"

using namespace nuansa::config;

namespace nuansa::services::auth {
    #ifdef DEBUG
    void AuthService::LogMutexState() const {
        LOG_DEBUG << "Auth mutex state - trying to acquire";
        if (std::unique_lock<std::timed_mutex> lock(authMutex, std::defer_lock); lock.try_lock()) {
            LOG_DEBUG << "Auth mutex is free";
            lock.unlock();
        } else {
            LOG_DEBUG << "Auth mutex is locked";
        }
    }
    #endif

    AuthService &AuthService::GetInstance() {
        static AuthService instance;
        return instance;
    }

    AuthService::AuthService() 
        : gen(rd()), 
          httpClient(std::make_unique<utils::HttpClient>()),
          tokenService_(std::make_unique<nuansa::services::token::TokenService>(
              Config::GetInstance().GetServerConfig().jwtSecret)) {
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
        try {
            LOG_DEBUG << "Attempting to acquire auth mutex for authentication";
            std::unique_lock<std::timed_mutex> lock(authMutex, std::defer_lock);
            if (!lock.try_lock_for(std::chrono::seconds(2))) {
                LOG_ERROR << "Failed to acquire mutex for authentication";
                return AuthResponse{false, "", "Server busy, please try again"};
            }
            LOG_DEBUG << "Auth mutex acquired for authentication";

            // Delegate authentication to UserService
            if (auto &userService = nuansa::services::user::UserService::GetInstance(); !userService.AuthenticateUser(
                request.GetUsername(), request.GetPassword())) {
                return nuansa::services::auth::AuthResponse{false, "", "Invalid credentials"};
            }

            // Generate tokens using TokenManager
            auto accessToken = tokenService_->GenerateAccessToken(request.GetUsername());
            auto refreshToken = tokenService_->GenerateRefreshToken(request.GetUsername());

            nlohmann::json tokenResponse = {
                {"access_token", accessToken.ToJson()},
                {"refresh_token", refreshToken.ToJson()}
            };

            return nuansa::services::auth::AuthResponse{true, tokenResponse.dump(), "Authentication successful"};
        } catch (const std::exception& e) {
            LOG_ERROR << "Authentication error: " << e.what();
            return AuthResponse{false, "", e.what()};
        }
    }

    void AuthService::Logout(const std::string& token) {
        tokenService_->LogoutToken(token);
    }

    std::optional<std::string> AuthService::GetUsernameFromToken(const std::string& token) {
        return tokenService_->GetUsernameFromToken(token);
    }

    nuansa::services::auth::AuthResponse AuthService::Register(const nuansa::services::auth::RegisterRequest &request) {
        LOG_DEBUG << "Registering user with provider: " << static_cast<int>(request.GetAuthProvider());

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
        try {
            LOG_DEBUG << "Attempting to acquire auth mutex for custom registration";
            std::unique_lock<std::timed_mutex> lock(authMutex, std::defer_lock);
            if (!lock.try_lock_for(std::chrono::seconds(2))) {
                LOG_ERROR << "Failed to acquire mutex for custom registration";
                return AuthResponse{false, "", "Server busy, please try again"};
            }
            LOG_DEBUG << "Auth mutex acquired for custom registration";

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

            // Generate tokens and save to repository
            auto token = tokenService_->GenerateAccessToken(*request.GetUsername());
            if (!tokenService_->SaveNewToken(token, *request.GetUsername(), "access")) {
                return AuthResponse{false, "", "Failed to create authentication token"};
            }

            return AuthResponse{true, token.ToJson().dump(), "Registration successful"};
        } catch (const std::exception& e) {
            LOG_ERROR << "Custom registration error: " << e.what();
            return AuthResponse{false, "", e.what()};
        }
    }

    AuthResponse AuthService::HandleOAuthRegistration(const RegisterRequest& request) {
        LOG_DEBUG << "Starting OAuth registration flow";

        if (!request.GetOAuthCredentials()) {
            LOG_ERROR << "Missing OAuth credentials";
            return AuthResponse{false, "", "Missing OAuth credentials"};
        }

        std::optional<OAuthUserInfo> userInfo;
        
        try {
            LOG_DEBUG << "Validating OAuth token for provider: " << static_cast<int>(request.GetAuthProvider());
            
            // Validate OAuth token and get user info first, without locking
            switch (request.GetAuthProvider()) {
                case AuthProvider::Google:
                    LOG_DEBUG << "Starting Google OAuth validation";
                    userInfo = ValidateGoogleToken(*request.GetOAuthCredentials());
                    break;
                case AuthProvider::GitHub:
                    LOG_DEBUG << "Starting GitHub OAuth validation";
                    userInfo = ValidateGitHubToken(*request.GetOAuthCredentials());
                    break;
                default:
                    LOG_ERROR << "Unsupported OAuth provider: " << static_cast<int>(request.GetAuthProvider());
                    return AuthResponse{false, "", "Unsupported OAuth provider"};
            }

            if (!userInfo) {
                LOG_ERROR << "Failed to validate OAuth token";
                return AuthResponse{false, "", "Failed to validate OAuth token"};
            }

            LOG_DEBUG << "Successfully validated OAuth token for user: " << userInfo->email;

            // Now acquire the lock only for the database operations
            LOG_DEBUG << "Attempting to acquire auth mutex";
            std::unique_lock<std::timed_mutex> lock(authMutex, std::defer_lock);
            
            // Reduce timeout to 2 seconds since we're only locking for DB operations
            if (!lock.try_lock_for(std::chrono::seconds(2))) {
                LOG_ERROR << "Failed to acquire mutex after 2 seconds timeout";
                return AuthResponse{false, "", "Server busy, please try again"};
            }
            
            LOG_DEBUG << "Auth mutex acquired";
            
            try {
                auto& userService = nuansa::services::user::UserService::GetInstance();
                bool userExists = userService.UserExists(userInfo->email);
                LOG_DEBUG << "User exists check completed: " << (userExists ? "true" : "false");

                if (!userExists) {
                    nuansa::models::User newUser{
                        userInfo->username,
                        userInfo->email,
                        "",  // No password for OAuth users
                        "",  // No salt needed
                        userInfo->picture
                    };
                    
                    if (!userService.CreateUser(newUser)) {
                        LOG_ERROR << "Failed to create user account";
                        return AuthResponse{false, "", "Failed to create user account"};
                    }
                    LOG_DEBUG << "Successfully created user account";
                }

                // Generate and save tokens
                auto accessToken = tokenService_->GenerateAccessToken(userInfo->email);
                auto refreshToken = tokenService_->GenerateRefreshToken(userInfo->email);

                bool accessTokenSaved = tokenService_->SaveNewToken(accessToken, userInfo->email, "access");
                bool refreshTokenSaved = tokenService_->SaveNewToken(refreshToken, userInfo->email, "refresh");

                if (!accessTokenSaved || !refreshTokenSaved) {
                    LOG_ERROR << "Failed to save authentication tokens";
                    return AuthResponse{false, "", "Failed to create authentication tokens"};
                }

                // Create response (can be done after releasing lock)
                nlohmann::json tokenResponse = {
                    {"access_token", accessToken.ToJson()},
                    {"refresh_token", refreshToken.ToJson()}
                };

                LOG_INFO << "OAuth registration successful for: " << userInfo->email;
                return AuthResponse{
                    true, 
                    tokenResponse.dump(),
                    "OAuth registration successful"
                };

            } catch (const std::exception& e) {
                LOG_ERROR << "Database operation failed: " << e.what();
                return AuthResponse{false, "", "Database operation failed: " + std::string(e.what())};
            }

        } catch (const std::exception& e) {
            LOG_ERROR << "OAuth registration error: " << e.what();
            return AuthResponse{false, "", std::string("OAuth registration failed: ") + e.what()};
        }
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
        LOG_DEBUG << "Starting GitHub token validation";
        try {
            if (!credentials.code || !credentials.redirectUri) {
                LOG_ERROR << "Missing required OAuth code or redirect URI";
                return std::nullopt;
            }

            // Exchange code for access token
            nlohmann::json requestBody = {
                {"client_id", Config::GetInstance().GetServerConfig().githubClientId},
                {"client_secret", Config::GetInstance().GetServerConfig().githubClientSecret},
                {"code", *credentials.code},
                {"redirect_uri", Config::GetInstance().GetServerConfig().githubRedirectUri}
            };

            LOG_DEBUG << "Exchanging code for token with body: " << requestBody.dump();
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
                         << " Response: " << tokenResponse.body
                         << " Status: " << tokenResponse.statusCode;
                return std::nullopt;
            }

            LOG_DEBUG << "Token exchange response: " << tokenResponse.body;

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
