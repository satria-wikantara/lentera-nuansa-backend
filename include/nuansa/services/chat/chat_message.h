//
// Created by I Gede Panca Sutresna on 07/12/24.
//

#ifndef NUANSA_SERVICES_CHAT_CHAT_MESSAGE_H
#define NUANSA_SERVICES_CHAT_CHAT_MESSAGE_H

#include "nuansa/utils/pch.h"
#include "nuansa/utils/exception/message_exception.h"
#include "nuansa/messages/base_message.h"

namespace nuansa::services::chat {
	struct ChatRequest final : public nuansa::messages::BaseMessage {
		ChatRequest() : header_(messages::MessageHeader()) {
		};

		ChatRequest(const messages::MessageHeader &header, std::string text,
		            std::vector<std::string> mentions, const bool edited,
		            const bool deleted)
			: header_(header),
			  text_(std::move(text)),
			  mentions_(std::move(mentions)),
			  edited_(edited),
			  deleted_(deleted) {
		};

		messages::MessageHeader GetMessageHeader() const {
			return header_;
		}

		std::string GetText() const {
			return text_;
		}

		std::vector<std::string> GetMentions() const {
			return mentions_;
		}

		bool IsEdited() const {
			return edited_;
		}

		bool IsDeleted() const {
			return deleted_;
		}

		// TODO: Where is the header?
		[[nodiscard]] nlohmann::json ToJson() const override {
			return nlohmann::json{
				{"text", text_},
				{"mentions", mentions_},
				{"edited", edited_},
				{"deleted", deleted_}
			};
		}

		static ChatRequest FromJson(const nlohmann::json &json) {
			try {
				ChatRequest request;
				request.header_ = messages::MessageHeader::FromJson(json["header"]);
				request.text_ = json["text"].get<std::string>();
				request.mentions_ = json["mentions"].get<std::vector<std::string> >();
				request.edited_ = json["edited"].get<bool>();
				request.deleted_ = json["deleted"].get<bool>();

				return request;
			} catch (std::exception &e) {
				throw nuansa::utils::exception::MessageParsingException(e.what());
			}
		}

		messages::MessageHeader header_;
		// TODO: add support for other message content like images, videos, files
		std::string text_;
		std::vector<std::string> mentions_; // metioned users
		bool edited_{false}; // edit status
		bool deleted_{false}; // deletion status
	};

	// TODO implement this
	struct ChatResponse final : public nuansa::messages::BaseMessage {
	};
}

#endif //NUANSA_SERVICES_CHAT_CHAT_MESSAGE_H
