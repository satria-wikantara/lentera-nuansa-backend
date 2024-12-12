#ifndef NUANSA_SERVICES_AUTH_AUTH_TYPES_H
#define NUANSA_SERVICES_AUTH_AUTH_TYPES_H

namespace nuansa::services::auth {
    enum class AuthProvider {
        Custom,
        Google,
        GitHub
    };

    struct OAuthCredentials {
        std::string accessToken;
        std::string refreshToken;
        std::string scope;
        int64_t expiresIn;

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
                {"expiresIn", expiresIn}
            };
        }

        static OAuthCredentials FromJson(const nlohmann::json& json) {
            return OAuthCredentials{
                json["accessToken"].get<std::string>(),
                json["refreshToken"].get<std::string>(),
                json["scope"].get<std::string>(),
                json["expiresIn"].get<int64_t>()
            };
        }
    };
}

#endif //NUANSA_SERVICES_AUTH_AUTH_TYPES_H 