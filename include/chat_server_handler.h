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
        explicit ChatServerHandler(std::shared_ptr<ChatServer> server);

        ChatServer& GetInstance();

        void BroadcastMessage(const std::string& sender, const std::string& msgData);

        void HandleMessage(const std::string& sender, const nlohmann::json& msgData);

        void HandleNewMessage(const std::string& sender, const nlohmann::json& msgData);

        void HandleEditMessage(const std::string& sender, const nlohmann::json& msgData);

        void HandleDeleteMessage(const std::string& sender, const nlohmann::json& msgData);

        void HandleDirectMessage(const std::string& sender, const nlohmann::json& msgData);

        void NotifyMentionedUsers(const Message& msg);

        std::vector<std::string> ExtractMentions(const std::string& content);

        void HandleWebSocketSession(std::shared_ptr<websocket::stream<tcp::socket>> ws);

    private:
        std::shared_ptr<ChatServer> chatServer;
    };
}

#endif /* CHAT_SERVER_HANDLER_H */
