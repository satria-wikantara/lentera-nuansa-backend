//
// Created by I Gede Panca Sutresna on 09/11/24.
//

#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <map>
#include <string>

#include "chat_types.h"

namespace App {
	class ChatServer {
	public:
		using ClientMap = std::map<std::string, ChatClient>;
		using MessageMap = std::map<std::string, Message>;

		static ChatServer &GetInstance();

		[[nodiscard]] const Message *GetMessage(const std::string &msgId) const {
			std::lock_guard<std::mutex> lock(messagesMutex);
			auto it = messages.find(msgId);
			return it != messages.end() ? &it->second : nullptr;
		}

		void SetMessage(const Message &msg) {
			std::lock_guard<std::mutex> lock(messagesMutex);
			messages[msg.id] = msg;
		}

		const MessageMap &GetMessages() const { return messages; }

	private:
		ClientMap clients;
		MessageMap messages;
		mutable std::mutex clientsMutex;
		mutable std::mutex messagesMutex;

		friend class ChatServerHandler;
	};
} // namespace App

#endif  // CHAT_SERVER_H
