#ifndef NUANSA_SERVICES_AUTH_AUTH_TYPES_H
#define NUANSA_SERVICES_AUTH_AUTH_TYPES_H

namespace nuansa::services::auth {
    enum class AuthProvider {
        Custom = 0,
        Google = 1,
        GitHub = 2
        // Add other providers as needed
    };

    struct OAuthCredentials {
        std::string accessToken;
        std::string refreshToken;
        std::string scope;
        int64_t expiresIn;
        
        // OAuth flow fields
        std::optional<std::string> code;        // Authorization code from OAuth provider
        std::optional<std::string> redirectUri; // Redirect URI used in OAuth flow
        
        // Provider-specific fields
        std::optional<std::string> idToken;     // Used by Google OAuth
        std::optional<std::string> tokenType;   // Used by GitHub OAuth
        std::optional<int64_t> expiresAt;      // Absolute expiration time

        [[nodiscard]] std::string ToJsonString() const {
            return ToJson().dump();
        }

        static OAuthCredentials FromJsonString(const std::string& jsonString) {
            return FromJson(nlohmann::json::parse(jsonString));
        }

        [[nodiscard]] nlohmann::json ToJson() const {
            return nlohmann::json{
                {"accessToken", accessToken},
                {"refreshToken", refreshToken},
                {"scope", scope},
                {"expiresIn", expiresIn},
                {"code", code.value_or(std::string())},
                {"redirectUri", redirectUri.value_or(std::string())},
                {"idToken", idToken.value_or(std::string())},
                {"tokenType", tokenType.value_or(std::string())},
                {"expiresAt", expiresAt.value_or(0)}
            };
        }

        static OAuthCredentials FromJson(const nlohmann::json& json) {
            OAuthCredentials creds{
                json["accessToken"].get<std::string>(),
                json["refreshToken"].get<std::string>(),
                json["scope"].get<std::string>(),
                json["expiresIn"].get<int64_t>()
            };

            // Handle optional fields
            if (json.contains("code")) {
                creds.code = json["code"].get<std::string>();
            }
            if (json.contains("redirectUri")) {
                creds.redirectUri = json["redirectUri"].get<std::string>();
            }
            if (json.contains("idToken")) {
                creds.idToken = json["idToken"].get<std::string>();
            }
            if (json.contains("tokenType")) {
                creds.tokenType = json["tokenType"].get<std::string>();
            }
            if (json.contains("expiresAt")) {
                creds.expiresAt = json["expiresAt"].get<int64_t>();
            }

            return creds;
        }
    };
}

#endif //NUANSA_SERVICES_AUTH_AUTH_TYPES_H 