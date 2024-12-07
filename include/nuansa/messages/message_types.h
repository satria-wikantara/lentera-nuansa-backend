#ifndef NUANSA_MESSAGES_MESSAGE_TYPES_H
#define NUANSA_MESSAGES_MESSAGE_TYPES_H

#include <utility>

#include "nuansa/utils/pch.h"
#include "nuansa/messages/base_message.h"


namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace nuansa::messages {
    struct Message {
        std::string id; // Unique message ID
        std::string sender; // Username of sender
        std::string content; // Message content
        std::vector<std::string> mentions; // Mentioned users
        std::time_t timestamp; // Message timestamp
        bool isEdited{false}; // Edit status
        bool isDeleted{false}; // Deletion status
    };

    enum class MessageType {
        New,
        Edit,
        Delete,
        DirectMessage,
        Login, // New
        Logout, // New
        AuthRequired, // New
        Plugin, // New
        Register
    };

    inline std::string MessageTypeToString(const MessageType messageType) {
        switch (messageType) {
            case MessageType::Register: return "register";
            case MessageType::New: return "new";
            case MessageType::Edit: return "edit";
            case MessageType::Delete: return "delete";
            case MessageType::DirectMessage: return "direct_message";
            case MessageType::Login: return "login";
            case MessageType::Logout: return "logout";
            case MessageType::AuthRequired: return "auth_required";
            case MessageType::Plugin: return "plugin";
            default: return "unknown";
        }
    }

    NLOHMANN_JSON_SERIALIZE_ENUM(MessageType, {
                                 {MessageType::New, "new"},
                                 {MessageType::Edit, "edit"},
                                 {MessageType::Delete, "delete"},
                                 {MessageType::DirectMessage, "direct_message"},
                                 {MessageType::Login, "login"},
                                 {MessageType::Logout, "logout"},
                                 {MessageType::AuthRequired, "auth_required"},
                                 {MessageType::Plugin, "plugin"},
                                 {MessageType::Register, "register"}
                                 });
}

#endif // NUANSA_MESSAGES_MESSAGE_TYPES_H
