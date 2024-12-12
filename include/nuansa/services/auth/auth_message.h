//
// Created by I Gede Panca Sutresna on 07/12/24.
//

#ifndef NUANSA_SERVICES_AUTH_AUTH_MESSAGE_H
#define NUANSA_SERVICES_AUTH_AUTH_MESSAGE_H

#include "nuansa/utils/pch.h"
#include "nuansa/utils/exception/message_exception.h"
#include "nuansa/messages/base_message.h"

namespace nuansa::services::auth {
	struct AuthRequest final : public nuansa::messages::BaseMessage {
		AuthRequest(std::string username,
		            std::string password)
			: username_(std::move(username)), password_(std::move(password)) {
		}

		[[nodiscard]] std::string GetUsername() const {
			return username_;
		}

		[[nodiscard]] std::string GetPassword() const {
			return password_;
		}

		[[nodiscard]] nlohmann::json ToJson() const override {
			return nlohmann::json{{"username", username_}, {"password", password_}};
		}

		static AuthRequest FromJson(const nlohmann::json &json) {
			try {
				return {json["username"].get<std::string>(), json["password"].get<std::string>()};
			} catch (std::exception &e) {
				throw nuansa::utils::exception::MessageParsingException(e.what());
			}
		}

		std::string username_;
		std::string password_;
	};


	struct AuthResponse final : public nuansa::messages::BaseMessage {
		AuthResponse(const bool success, std::string token, std::string message)
			: success_(success), token_(std::move(token)), message_(std::move(message)) {
		}

		[[nodiscard]] bool IsSuccess() const {
			return success_;
		}

		[[nodiscard]] std::string GetToken() const {
			return token_;
		}

		[[nodiscard]] std::string GetMessage() const {
			return message_;
		}

		[[nodiscard]] nlohmann::json ToJson() const override {
			return nlohmann::json{{"success", success_}, {"token", token_}, "message", message_};
		}

		static AuthResponse FromJson(const nlohmann::json &json) {
			try {
				return {
					json["success"].get<bool>(), json["token"].get<std::string>(),
					json["message"].get<std::string>()
				};
			} catch (std::exception &e) {
				throw nuansa::utils::exception::MessageParsingException(e.what());
			}
		}

		bool success_;
		std::string token_;
		std::string message_;
	};
}

#endif //NUANSA_SERVICES_AUTH_AUTH_MESSAGE_H
