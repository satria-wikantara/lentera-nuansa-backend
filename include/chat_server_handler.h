#ifndef CHAT_SERVER_HANDLER_H
#define CHAT_SERVER_HANDLER_H


#include <boost/beast/websocket.hpp>
#include <string>
#include <nlohmann/adl_serializer.hpp>

#include "chat_types.h"
#include "chat_server.h"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace App {
    class ChatServerHandler {
    public:
        static ChatServer& GetInstance();

        static void BroadcastMessage(const std::string& sender, const std::string& msgData);

        static void HandleMessage(const std::string& sender, const nlohmann::json& msgData);

        static void HandleNewMessage(const std::string& sender, const nlohmann::json& msgData);

        static void HandleEditMessage(const std::string& sender, const nlohmann::json& msgData);

        static void HandleDeleteMessage(const std::string& sender, const nlohmann::json& msgData);

        static void HandleDirectMessage(const std::string& sender, const nlohmann::json& msgData);

        static void NotifyMentionedUsers(const Message& msg);

        static std::vector<std::string> ExtractMentions(const std::string& content);

        static void HandleWebSocketSession(std::shared_ptr<websocket::stream<tcp::socket>> ws);

    private:
        static ChatServer chatServer;
        ChatServerHandler() = delete; // Prevent instantiation
    };
}

#endif /* CHAT_SERVER_HANDLER_H */
