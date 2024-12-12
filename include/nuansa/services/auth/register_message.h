//
// Created by I Gede Panca Sutresna on 07/12/24.
//

#ifndef NUANSA_SERVICES_AUTH_REGISTER_MESSAGE_H
#define NUANSA_SERVICES_AUTH_REGISTER_MESSAGE_H

#include "nuansa/utils/pch.h"
#include "nuansa/utils/exception/message_exception.h"
#include "nuansa/messages/base_message.h"
#include "nuansa/services/auth/auth_types.h"

using namespace nuansa::utils::common;

namespace nuansa::services::auth {
	struct RegisterRequest final : public nuansa::messages::BaseMessage {
		RegisterRequest() : messageHeader_(messages::MessageHeader()), 
						  authProvider_(AuthProvider::Custom) {
		}

		RegisterRequest(const messages::MessageHeader& messageHeader,
					   const std::string& username,
					   const std::string& email,
					   const std::string& password,
					   AuthProvider authProvider = AuthProvider::Custom)
			: messageHeader_(messageHeader),
			  authProvider_(authProvider),
			  username_(username),
			  email_(email),
			  password_(password) {
		}

		RegisterRequest(const messages::MessageHeader& messageHeader,
					   AuthProvider authProvider,
					   const OAuthCredentials& oauthCredentials)
			: messageHeader_(messageHeader),
			  authProvider_(authProvider),
			  oauthCredentials_(oauthCredentials) {
		}

		messages::MessageHeader GetMessageHeader() const {
			return messageHeader_;
		}

		AuthProvider GetAuthProvider() const {
			return authProvider_;
		}

		std::optional<OAuthCredentials> GetOAuthCredentials() const {
			return oauthCredentials_;
		}

		std::optional<std::string> GetUsername() const {
			return username_;
		}

		std::optional<std::string> GetEmail() const {
			return email_;
		}

		std::optional<std::string> GetPassword() const {
			return password_;
		}

		[[nodiscard]] nlohmann::json ToJson() const override {
			nlohmann::json json;
			json[MESSAGE_HEADER] = messageHeader_.ToJson();
			json["authProvider"] = static_cast<int>(authProvider_);
			
			if (oauthCredentials_) {
				json["oauthCredentials"] = {
					{"accessToken", oauthCredentials_->accessToken},
					{"refreshToken", oauthCredentials_->refreshToken},
					{"scope", oauthCredentials_->scope},
					{"expiresIn", oauthCredentials_->expiresIn}
				};
			}
			
			if (username_) json["username"] = *username_;
			if (email_) json["email"] = *email_;
			if (password_) json["password"] = *password_;
			
			return json;
		}

		static RegisterRequest FromJson(const nlohmann::json& json) {
			try {
				RegisterRequest request;
				request.messageHeader_ = messages::MessageHeader::FromJson(json[MESSAGE_HEADER]);
				request.authProvider_ = static_cast<AuthProvider>(json["authProvider"].get<int>());

				if (json.contains("oauthCredentials")) {
					const auto& oauth = json["oauthCredentials"];
					request.oauthCredentials_ = OAuthCredentials{
						oauth["accessToken"].get<std::string>(),
						oauth["refreshToken"].get<std::string>(),
						oauth["scope"].get<std::string>(),
						oauth["expiresIn"].get<int64_t>()
					};
				}

				if (json.contains("username")) 
					request.username_ = json["username"].get<std::string>();
				if (json.contains("email")) 
					request.email_ = json["email"].get<std::string>();
				if (json.contains("password")) 
					request.password_ = json["password"].get<std::string>();

				return request;
			} catch (const std::exception& e) {
				throw nuansa::utils::exception::MessageParsingException(e.what());
			}
		}

		messages::MessageHeader messageHeader_;
		AuthProvider authProvider_;
		std::optional<OAuthCredentials> oauthCredentials_;
		std::optional<std::string> username_;
		std::optional<std::string> email_;
		std::optional<std::string> password_;
	};

	struct RegisterResponse final : public nuansa::messages::BaseMessage {
		RegisterResponse(const bool success, std::string message)
			: success_(success), message_(std::move(message)) {
		}

		bool IsSuccess() const {
			return success_;
		}

		std::string GetMessage() const {
			return message_;
		}

		[[nodiscard]] nlohmann::json ToJson() const override {
			return nlohmann::json{{"success", success_}, {"message", message_}};
		}

		static RegisterResponse FromJson(const nlohmann::json &json) {
			try {
				return RegisterResponse(json["success"].get<bool>(), json["message"].get<std::string>());
			} catch (std::exception &e) {
				throw nuansa::utils::exception::MessageParsingException(e.what());
			}
		}

		// TODO: Add headers to the request
		bool success_;
		std::string message_;
	};
}

#endif //NUANSA_SERVICES_AUTH_REGISTER_MESSAGE_H
