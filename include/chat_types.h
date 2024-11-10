#ifndef CHAT_TYPES_H
#define CHAT_TYPES_H

#include <string>
#include <map>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <vector>
#include "nlohmann/json.hpp"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace App {
    struct ChatClient {
        std::string username;
        std::shared_ptr<websocket::stream<tcp::socket>> ws;

        std::shared_ptr<websocket::stream<tcp::socket>> GetWebSocket() const {
            return ws;
        }
    };

    struct Message {
        std::string id;                     // unique message ID
        std::string sender;                 // Username of sender
        std::string content;                // Message content
        std::vector<std::string> mentions;  // Mentioned users
        std::time_t timestamp;              // Message timestamp
        bool isEdited{false};               // Edit status
        bool isDeleted{false};              // Deletion status
    };

    enum class MessageType {
        New,
        Edit,
        Delete,
        DirectMessage
    };

    inline MessageType StringToMessageType(const std::string& type) {
        if (type == "new") return MessageType::New;
        if (type == "edit") return MessageType::Edit;
        if (type == "delete") return MessageType::Delete;
        if (type == "direct_message") return MessageType::DirectMessage;
        throw std::invalid_argument("Invalid message type: " + type);
    }

    NLOHMANN_JSON_SERIALIZE_ENUM(MessageType, {
        {MessageType::New, "new"},
        {MessageType::Edit, "edit"},
        {MessageType::Delete, "delete"},
        {MessageType::DirectMessage, "direct_message"}
    });

}

#endif /* CHAT_TYPES_H */
