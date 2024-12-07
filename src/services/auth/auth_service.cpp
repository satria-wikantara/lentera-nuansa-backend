//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/pch/pch.h"

#include "nuansa/services/auth/auth_service.h"
#include "nuansa/messages/message_types.h"
#include "nuansa/services/user/user_service.h"
#include "nuansa/utils/validation.h"

namespace nuansa::services::auth {
    AuthService &AuthService::GetInstance() {
        static AuthService instance;
        return instance;
    }

    AuthService::AuthService() : gen(rd()) {
        // TODO: In production, load from secure database
        // For demonstration, adding some test users with hashed passwords
        userCredentials["alice"] = crypt("password123", "$6$random_salt");
        userCredentials["bob"] = crypt("password456", "$6$random_salt");
    }

    nuansa::messages::AuthResponse AuthService::Authenticate(const nuansa::messages::AuthRequest &request) {
        std::lock_guard<std::mutex> lock(authMutex);

        // Delegate authentication to UserService
        if (auto &userService = nuansa::services::user::UserService::GetInstance(); !userService.AuthenticateUser(
            request.username, request.password)) {
            return nuansa::messages::AuthResponse{false, "", "Invalid credentials"};
        }

        // Generate new token
        const std::string token = GenerateToken(request.username);

        // Store token info
        const TokenInfo tokenInfo{
            request.username,
            std::chrono::system_clock::now() + std::chrono::hours(24)
        };
        activeTokens[token] = tokenInfo;

        return nuansa::messages::AuthResponse{true, token, "Authentication successful"};
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

    nuansa::messages::AuthResponse AuthService::Register(const nuansa::messages::RegisterRequest &request) {
        std::lock_guard<std::mutex> lock(authMutex);

        // Validate username
        if (!nuansa::utils::Validation::ValidateUsername(request.username)) {
            LOG_WARNING << "Invalid username format: " << request.username;
            return nuansa::messages::AuthResponse{false, "", "Invalid username format"};
        }

        // Validate email
        if (!nuansa::utils::Validation::ValidateEmail(request.email)) {
            LOG_WARNING << "Invalid email format: " << request.email;
            return nuansa::messages::AuthResponse{false, "", "Invalid email format"};
        }

        // Validate password
        if (!nuansa::utils::Validation::ValidatePassword(request.password)) {
            LOG_WARNING << "Invalid password format";
            return nuansa::messages::AuthResponse{false, "", "Invalid password format"};
        }

        // Delegate registration to UserService
        auto &userService = nuansa::services::user::UserService::GetInstance();

        const std::string salt = nuansa::models::User::GenerateSalt();
        if (const std::string hashedPassword = nuansa::utils::crypto::CryptoUtil::HashPassword(request.password, salt);
            !userService.CreateUser(nuansa::models::User{request.username, request.email, hashedPassword, salt})) {
            return nuansa::messages::AuthResponse{false, "", "Registration failed"};
        }

        // Generate new token for the newly registered user
        const std::string token = GenerateToken(request.username);

        // Store token info
        const TokenInfo tokenInfo{
            request.username,
            std::chrono::system_clock::now() + std::chrono::hours(24)
        };
        activeTokens[token] = tokenInfo;

        return nuansa::messages::AuthResponse{true, token, "Registration successful"};
    }
}
