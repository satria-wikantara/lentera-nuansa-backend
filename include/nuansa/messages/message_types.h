#ifndef NUANSA_MESSAGES_MESSAGE_TYPES_H
#define NUANSA_MESSAGES_MESSAGE_TYPES_H

#include "nuansa/utils/pch.h"
#include "nuansa/utils/crypto/crypto_util.h"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace nuansa::messages {
    struct MessageHeader {
        std::string version{"1.0"}; // Version of the message format
        std::string messageId; // Unique identifier for the message
        std::time_t timestamp; // Timestamp of the message
        std::string messageType; // Type/category of the message
        std::string correlationId; // ID to correlate request/response pairs
        int priority{0}; // Message priority level
        std::string contentType{"application/json"}; // Format of the payload (e.g., "application/json")
        std::string encoding{"UTF-8"}; // Character encoding
        size_t messageLength; // Length of the message
        std::string messageHash; // SHA-256 hash of the message
        std::map<std::string, std::string> customHeaders; // Extension headers

        explicit MessageHeader(
            const std::string &version = "1.0",
            const std::string &messageId = "",
            const std::time_t timestamp = std::time(nullptr),
            const std::string &messageType = "",
            const std::string &correlationId = "",
            const int priority = 0,
            const std::string &contentType = "",
            const std::string &encoding = "UTF-8",
            const size_t messageLength = 0,
            const std::string &messageHash = "",
            const std::map<std::string, std::string> &customHeaders = {}
        ) : version(version),
            messageId(messageId),
            timestamp(timestamp),
            messageType(messageType),
            correlationId(correlationId),
            priority(priority),
            contentType(contentType),
            encoding(encoding),
            messageLength(messageLength),
            messageHash(messageHash),
            customHeaders(customHeaders) {
        }

        // Convert JSON to MessageHeader
        static MessageHeader FromJson(const nlohmann::json &json) {
            MessageHeader header;
            header.version = json.value("version", std::string{});
            header.messageId = json.value("messageId", std::string{});
            header.timestamp = json.value("timestamp", std::time_t{});
            header.messageType = json.value("messageType", std::string{});
            header.correlationId = json.value("correlationId", std::string{});
            header.priority = json.value("priority", 0);
            header.contentType = json.value("contentType", std::string{"application/json"});
            header.encoding = json.value("encoding", std::string{"UTF-8"});
            header.messageLength = json.value("messageLength", 0);
            header.messageHash = json.value("messageHash", std::string{});
            header.customHeaders = json.value("customHeaders", std::map<std::string, std::string>{});

            return header;
        }

    protected:
        void UpdateHeaders(const std::string &content) {
            timestamp = std::time(nullptr);
            messageLength = content.length();
            messageHash = nuansa::utils::crypto::CryptoUtil::GenerateSHA256Hash(content);
        }
    };

    struct Message {
        std::string id; // Unique message ID
        std::string sender; // Username of sender
        std::string content; // Message content
        std::vector<std::string> mentions; // Mentioned users
        std::time_t timestamp; // Message timestamp
        bool isEdited{false}; // Edit status
        bool isDeleted{false}; // Deletion status
    };

    struct AuthRequest {
        std::string username;
        std::string password;
    };

    struct AuthResponse {
        bool success;
        std::string token;
        std::string message; // For error messages or success confirmations
    };

    struct RegisterRequest : public MessageHeader {
        std::string username;
        std::string email;
        std::string password;
    };

    struct RegisterResponse : public MessageHeader {
        // TODO: Add headers to the response, that contain version, timestamp, message_length and message_hash
        std::string type;
        bool success;
        std::string message;

        // Convert to JSON
        nlohmann::json ToJson() const {
            return {
                {"type", type},
                {"success", success},
                {"message", message}
            };
        }
    };

    enum class AuthStatus {
        Authenticated,
        InvalidCredentials,
        TokenExpired,
        NotAuthenticated
    };

    inline AuthStatus StringToAuthStatus(const std::string &status) {
        if (status == "authenticated") return AuthStatus::Authenticated;
        if (status == "invalid_credentials") return AuthStatus::InvalidCredentials;
        if (status == "token_expired") return AuthStatus::TokenExpired;
        if (status == "not_authenticated") return AuthStatus::NotAuthenticated;
        throw std::invalid_argument("Invalid auth status: " + status);
    }

    NLOHMANN_JSON_SERIALIZE_ENUM(AuthStatus, {
                                 {AuthStatus::Authenticated, "authenticated"},
                                 {AuthStatus::InvalidCredentials, "invalid_credentials"},
                                 {AuthStatus::TokenExpired, "token_expired"},
                                 {AuthStatus::NotAuthenticated, "not_authenticated"}
                                 });


    enum class MessageType {
        New,
        Edit,
        Delete,
        DirectMessage,
        Login, // New
        Logout, // New
        AuthRequired, // New
        Plugin, // New
        Register // New
    };

    inline MessageType StringToMessageType(const std::string &type) {
        if (type == "new") return MessageType::New;
        if (type == "edit") return MessageType::Edit;
        if (type == "delete") return MessageType::Delete;
        if (type == "direct_message") return MessageType::DirectMessage;
        if (type == "login") return MessageType::Login;
        if (type == "logout") return MessageType::Logout;
        if (type == "auth_required") return MessageType::AuthRequired;
        if (type == "plugin") return MessageType::Plugin;
        if (type == "register") return MessageType::Register;
        throw std::invalid_argument("Invalid message type: " + type);
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

    inline std::string ToString(MessageType type) {
        switch (type) {
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
}

#endif // NUANSA_MESSAGES_MESSAGE_TYPES_H
