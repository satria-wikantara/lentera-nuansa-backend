#ifndef NUANSA_MESSAGES_MESSAGE_HEADER_H
#define NUANSA_MESSAGES_MESSAGE_HEADER_H

#include "nuansa/utils/pch.h"

namespace nuansa::messages {
    class MessageHeader {
    public:
        MessageHeader() = default;
        
        [[nodiscard]] nlohmann::json ToJson() const {
            return nlohmann::json{
                {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
                {"version", "1.0"}
            };
        }

        static MessageHeader FromJson(const nlohmann::json& json) {
            return MessageHeader();  // For now, just return default
        }

    };
}

#endif //NUANSA_MESSAGES_MESSAGE_HEADER_H 