//
// Created by I Gede Panca Sutresna on 09/11/24.
//

#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <string>
#include <map>
#include "chat_types.h"

namespace App {

    class ChatServer {

    public:
        using ClientMap = std::map<std::string, ChatClient>;
        using MessageMap = std::map<std::string, Message>;

        static ChatServer& GetInstance();

        [[nodiscard]] const Message* GetMessage(const std::string& msgId) const {
            std::lock_guard<std::mutex> lock(messagesMutex);
            auto it = messages.find(msgId);
            return it != messages.end() ? &it->second : nullptr;
        }

    private:
        ClientMap clients;
        MessageMap messages;
        mutable std::mutex clientsMutex;
        mutable std::mutex messagesMutex;

        friend class ChatServerHandler;
    };
}

#endif //CHAT_SERVER_H
