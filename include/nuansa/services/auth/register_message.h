//
// Created by I Gede Panca Sutresna on 07/12/24.
//

#ifndef NUANSA_SERVICES_AUTH_REGISTER_MESSAGE_H
#define NUANSA_SERVICES_AUTH_REGISTER_MESSAGE_H

#include "nuansa/utils/pch.h"
#include "nuansa/utils/exception/message_exception.h"
#include "nuansa/messages/base_message.h"

using namespace nuansa::utils::common;

namespace nuansa::services::auth {
	struct RegisterRequest final : public nuansa::messages::BaseMessage {
		RegisterRequest() : messageHeader_(messages::MessageHeader()) {
		}

		RegisterRequest(const messages::MessageHeader &messageHeader,
		                std::string username, std::string email,
		                std::string password)
			: messageHeader_(messageHeader), username_(std::move(username)), email_(std::move(email)),
			  password_(std::move(password)) {
		}

		messages::MessageHeader GetMessageHeader() const {
			return messageHeader_;
		}

		std::string GetUsername() const {
			return username_;
		}

		std::string GetEmail() const {
			return email_;
		}

		std::string GetPassword() const {
			return password_;
		}

		// TODO: Where is the messageHeader?
		[[nodiscard]] nlohmann::json ToJson() const override {
			return nlohmann::json{{"username", username_}, {"email", email_}, {"password", password_}};
		}

		static RegisterRequest FromJson(nlohmann::json &json) {
			try {
				RegisterRequest request;
				request.messageHeader_ = messages::MessageHeader::FromJson(json[MESSAGE_HEADER]);
				request.username_ = json["username"].get<std::string>();
				request.email_ = json["email"].get<std::string>();
				request.password_ = json["password"].get<std::string>();
				return request;
			} catch (std::exception &e) {
				throw nuansa::utils::exception::MessageParsingException(e.what());
			}
		}

		messages::MessageHeader messageHeader_;
		std::string username_;
		std::string email_;
		std::string password_;
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
