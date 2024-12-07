//
// Created by I Gede Panca Sutresna on 03/12/24.
//

#ifndef NUANSA_MESSAGES_CORE_MESSAGE_H
#define NUANSA_MESSAGES_CORE_MESSAGE_H

#include "nuansa/utils/pch.h"

namespace nuansa::messages {
	struct BaseMessage {
		virtual nlohmann::json to_json() const = 0;

		virtual ~BaseMessage() = default;
	};

	struct CoreAuthMessage : BaseMessage {
		std::string username;

		CoreAuthMessage() = default;

		CoreAuthMessage(const std::string &username) : username(username) {
		};

		nlohmann::json to_json() const override {
			return {{"username", username}};
		}

		static CoreAuthMessage from_json(const nlohmann::json &j) {
			try {
				return CoreAuthMessage(j["username"].get<std::string>());
			} catch (const std::exception &e) {
				throw std::runtime_error("Failed to parse CoreAuthMessage from JSON: " + std::string(e.what()));
			}
		}
	};
}

#endif //NUANSA_MESSAGES_CORE_MESSAGE_H
