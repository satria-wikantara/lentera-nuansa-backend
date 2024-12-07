#ifndef NUANSA_WEBSOCKET_HANDLER_H
#define NUANSA_WEBSOCKET_HANDLER_H

#include "nuansa/utils/pch.h"
#include "nuansa/handler/websocket_client.h"
#include "nuansa/messages/message_types.h"
#include "nuansa/handler/websocket_server.h"

namespace nuansa::handler {
    class WebSocketHandler {
    public:
        explicit WebSocketHandler(const std::shared_ptr<WebSocketServer> &server);

        void HandleSession(std::shared_ptr<websocket::stream<tcp::socket> > ws);

        static void SendMessage(const std::shared_ptr<WebSocketClient> &client, const std::string &message);

        static void SendMessage(std::shared_ptr<WebSocketClient> client, const nlohmann::json &jsonMessage);

        void BroadcastMessage(const std::string &sender, const std::string &message) const;

        static void SendSystemMessage(const std::shared_ptr<WebSocketClient> &client, const std::string &message);

        static void SendAckMessage(const std::shared_ptr<WebSocketClient> &client, const std::string &messageId,
                                   bool success,
                                   const std::string &details = "");

        void SendTypingNotification(const std::string &username, bool isTyping,
                                    const std::string &recipient = "") const;

        void SendOnlineUsersList(const std::shared_ptr<WebSocketClient> &client) const;

        bool IsUserOnline(const std::string &username) const;

        std::size_t GetOnlineUserCount() const;

        std::vector<std::string> GetOnlineUsers() const;

        void HandleClientDisconnection(const std::shared_ptr<WebSocketClient> &client) const;

        static void SendErrorMessage(const std::shared_ptr<WebSocketClient> &client, const std::string &errorMessage,
                                     const std::string &errorCode = "");

        static std::vector<std::string> ExtractMentions(const std::string &content);

        void NotifyMentionedUsers(const nuansa::messages::Message &msg) const;

        static void ValidateMessageFormat(const nlohmann::json &msgData);

        static void SendAuthRequiredMessage(const std::shared_ptr<WebSocketClient> &client);

    private:
        std::shared_ptr<WebSocketServer> websocketServer;
    };
} // namespace nuansa::handler

#endif // NUANSA_WEBSOCKET_HANDLER_H
