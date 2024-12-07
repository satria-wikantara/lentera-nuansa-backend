#ifndef NUANSA_WEBSOCKET_HANDLER_H
#define NUANSA_WEBSOCKET_HANDLER_H

#include "nuansa/pch/pch.h"
#include "nuansa/handler/websocket_client.h"
#include "nuansa/messages/message_types.h"
#include "nuansa/handler/websocket_server.h"

namespace nuansa::handler {
    class WebSocketHandler {
    public:
        explicit WebSocketHandler(std::shared_ptr<WebSocketServer> server);

        void HandleSession(std::shared_ptr<websocket::stream<tcp::socket> > ws);

        void SendMessage(std::shared_ptr<WebSocketClient> client, const std::string &message);

        void SendMessage(std::shared_ptr<WebSocketClient> client, const nlohmann::json &jsonMessage);

        void BroadcastMessage(const std::string &sender, const std::string &message);

        void SendSystemMessage(std::shared_ptr<WebSocketClient> client, const std::string &message);

        void SendAckMessage(std::shared_ptr<WebSocketClient> client, const std::string &messageId, bool success,
                            const std::string &details = "");

        void SendTypingNotification(const std::string &username, bool isTyping, const std::string &recipient = "");

        void SendOnlineUsersList(std::shared_ptr<WebSocketClient> client);

        bool IsUserOnline(const std::string &username);

        std::size_t GetOnlineUserCount();

        std::vector<std::string> GetOnlineUsers();

        void HandleClientDisconnection(std::shared_ptr<WebSocketClient> client);

        void SendErrorMessage(std::shared_ptr<WebSocketClient> client, const std::string &errorMessage,
                              const std::string &errorCode = "");

        std::vector<std::string> ExtractMentions(const std::string &content);

        void NotifyMentionedUsers(const nuansa::messages::Message &msg);

        void ValidateMessageFormat(const nlohmann::json &msgData);

        void SendAuthRequiredMessage(std::shared_ptr<WebSocketClient> client);

    private:
        std::shared_ptr<WebSocketServer> websocketServer;
    };
} // namespace nuansa::handler

#endif // NUANSA_WEBSOCKET_HANDLER_H
