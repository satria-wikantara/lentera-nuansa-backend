#ifndef APP_CHAT_SERVER_H
#define APP_CHAT_SERVER_H

#include "chat_types.h"
#include <string>

namespace App {
    class ChatServerHandler {
    public:
        static ChatServer& GetInstance();
        static void BroadcastMessage(const std::string& sender, const std::string& message);
        static void HandleWebSocketSession(std::shared_ptr<websocket::stream<tcp::socket>> ws);

    private:
        static ChatServer chatServer;
    };
}

#endif /* APP_CHAT_SERVER_H */
