//
// Created by I Gede Panca Sutresna on 03/12/24.
//

#ifndef NUANSA_MESSAGES_CORE_MESSAGE_H
#define NUANSA_MESSAGES_CORE_MESSAGE_H

#include "nuansa/utils/pch.h"
#include "nuansa/utils/crypto/crypto_util.h"

using namespace nuansa::utils::common;

namespace nuansa::messages {
    struct MessageHeader {
        std::string version{"1.0"}; // Version of the message format
        std::string messageId; // Unique identifier for the message
        std::string sender;
        std::time_t timestamp; // Timestamp of the message received on the server
        std::string messageType; // Type/category of the message
        std::string correlationId; // ID to correlate request/response pairs
        int priority{0}; // Message priority level
        std::string contentType{"application/json"}; // Format of the payload (e.g., "application/json")
        std::string encoding{"UTF-8"}; // Character encoding
        size_t messageLength; // Length of the message
        std::string messageHash; // SHA-256 hash of the message
        std::map<std::string, std::string> customHeaders; // Extension headers

        explicit MessageHeader(
            std::string version = "1.0",
            std::string messageId = "",
            std::string sender = "",
            const std::time_t timestamp = std::time(nullptr),
            std::string messageType = "",
            std::string correlationId = "",
            const int priority = 0,
            std::string contentType = "",
            std::string encoding = "UTF-8",
            const size_t messageLength = 0,
            std::string messageHash = "",
            const std::map<std::string, std::string> &customHeaders = {}
        ) : version(std::move(version)),
            messageId(std::move(messageId)),
            timestamp(timestamp),
            messageType(std::move(messageType)),
            correlationId(std::move(correlationId)),
            priority(priority),
            contentType(std::move(contentType)),
            encoding(std::move(encoding)),
            messageLength(messageLength),
            messageHash(std::move(messageHash)),
            customHeaders(customHeaders) {
        }

        // Convert JSON to MessageHeader
        static MessageHeader FromJson(const nlohmann::json &json) {
            MessageHeader header;
            header.version = json.value(MESSAGE_HEADER_VERSION, std::string{});
            header.messageId = json.value(MESSAGE_HEADER_MESSAGE_ID, std::string{});
            header.sender = json.value(MESSAGE_HEADER_SENDER, std::string{});
            header.timestamp = json.value(MESSAGE_HEADER_TIMESTAMP, std::time_t{});
            header.messageType = json.value(MESSAGE_HEADER_MESSAGE_TYPE, std::string{});
            header.correlationId = json.value(MESSAGE_HEADER_CORRELATION_ID, std::string{});
            header.priority = json.value(MESSAGE_HEADER_PRIORITY, 0);
            header.contentType = json.value(MESSAGE_HEADER_CONTENT_TYPE, std::string{"application/json"});
            header.encoding = json.value(MESSAGE_HEADER_ENCODING, std::string{"UTF-8"});
            header.messageLength = json.value(MESSAGE_HEADER_CONTENT_LENGTH, 0);
            header.messageHash = json.value(MESSAGE_HEADER_HASH, std::string{});
            header.customHeaders = json.value(MESSAGE_HEADER_CUSTOM_HEADERS, std::map<std::string, std::string>{});

            return header;
        }

    protected:
        void UpdateHeaders(const std::string &content) {
            timestamp = std::time(nullptr);
            messageLength = content.length();
            messageHash = nuansa::utils::crypto::CryptoUtil::GenerateSHA256Hash(content);
        }
    };

    struct BaseMessage {
        virtual ~BaseMessage() = default;

        [[nodiscard]] virtual nlohmann::json ToJson() const = 0;

        static std::unique_ptr<BaseMessage> FromJson(const nlohmann::json &json);
    };
}

#endif //NUANSA_MESSAGES_CORE_MESSAGE_H
