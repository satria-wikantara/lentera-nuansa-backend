

#include <iostream>
#include <regex>
#include <chrono>
#include <boost/beast/core.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <nlohmann/json.hpp>

#include "../include/chat_server_handler.h"
#include "../include/chat_server.h"
#include "../include/global_config.h"

namespace App {
    ChatServer ChatServerHandler::chatServer;

    ChatServer& ChatServerHandler::GetInstance() {
        return chatServer;
    }

    void ChatServerHandler::HandleWebSocketSession(std::shared_ptr<websocket::stream<tcp::socket>> ws) {
        try {
            ws->accept();
            BOOST_LOG_TRIVIAL(info) << "New websocket connection accepted";

            beast::flat_buffer buffer;
            ws->read(buffer);
            std::string auth_message = beast::buffers_to_string(buffer.data());

            nlohmann::json auth_data = nlohmann::json::parse(auth_message);
            std::string username = auth_data["username"];

            BOOST_LOG_TRIVIAL(debug) << "User authentication: " << username;

            {
                std::lock_guard<std::mutex> lock(chatServer.clientsMutex);
                chatServer.clients[username] = ChatClient{username, ws};
                BOOST_LOG_TRIVIAL(debug) << "User " << username << " added to active clients";
            }

            BroadcastMessage("system", username + " has joined the chat");

            while (true) {
                buffer.consume(buffer.size());
                ws->read(buffer);
                std::string message = beast::buffers_to_string(buffer.data());

                try {
                    nlohmann::json msgData = nlohmann::json::parse(message);
                    HandleMessage(username, msgData);
                } catch (const nlohmann::json::exception& e) {
                    BOOST_LOG_TRIVIAL(error) << "JSON parsing error: " << e.what();
                }
            }
        } catch (beast::system_error const& se) {
            if (se.code() != websocket::error::closed) {
                BOOST_LOG_TRIVIAL(error) << "WebSocket error: " << se.code().message();
            }
        } catch (std::exception const& e) {
            BOOST_LOG_TRIVIAL(error) << "Error: " << e.what();
        }

        // Handle disconnection
        std::string username;
        {
            std::lock_guard<std::mutex> lock(chatServer.clientsMutex);
            for (auto it = chatServer.clients.begin(); it != chatServer.clients.end(); ++it) {
                if (it->second.GetWebSocket().get() == ws.get()) {
                    username = it->first;
                    chatServer.clients.erase(it);
                    BOOST_LOG_TRIVIAL(debug) << "User " << username << " disconnected";
                    break;
                }
            }
        }

        if (!username.empty()) {
            BroadcastMessage("system", username + " has left the chat");
        }
    }

    void ChatServerHandler::BroadcastMessage(const std::string& sender, const std::string& message) {
        std::lock_guard<std::mutex> lock(chatServer.clientsMutex);
        nlohmann::json messageObj = {
            {"type", "message"},
            {"sender", sender},
            {"content", message}
        };
        std::string messageStr = messageObj.dump();

        for (auto& [username, client] : chatServer.clients) {
            try {
                client.ws->text(true);
                client.ws->write(net::buffer(messageStr));
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << "Error broadcasting to " << username << ": " << e.what();
            }
        }
    }

    void ChatServerHandler::HandleMessage(const std::string& sender, const nlohmann::json& msgData) {

        BOOST_LOG_TRIVIAL(debug) << "Received message from " << sender << ": " << msgData.dump(2);

        try {
            MessageType type = msgData["type"].get<MessageType>();
            BOOST_LOG_TRIVIAL(debug) << "Processing message type: " << msgData["type"];

            switch (type) {
                case MessageType::New:
                    HandleNewMessage(sender, msgData);
                    break;
                case MessageType::Edit:
                    HandleEditMessage(sender, msgData);
                    break;
                case MessageType::Delete:
                    HandleDeleteMessage(sender, msgData);
                    break;
                case MessageType::DirectMessage:
                    HandleDirectMessage(sender, msgData);
                    break;
            }
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Error processing message: " << e.what();
        }
    }

    void ChatServerHandler::HandleNewMessage(const std::string& sender, const nlohmann::json& msgData) {

        BOOST_LOG_TRIVIAL(debug) << "Creating new message from " << sender;

        boost::uuids::random_generator gen;
        Message msg {
            .id = boost::uuids::to_string(gen()),
            .sender = sender,
            .content = msgData["content"],
            .mentions = ExtractMentions(msgData["content"]),
            .timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())
        };

        BOOST_LOG_TRIVIAL(debug) << "Generated message ID: " << msg.id;

        // Store message in history
        {
            std::lock_guard<std::mutex> lock(chatServer.messagesMutex);
            chatServer.messages[msg.id] = msg;
            BOOST_LOG_TRIVIAL(debug) << "Message stored in history";
        }

        //TODO: do something when mentions is found
        if (!msg.mentions.empty()) {
            BOOST_LOG_TRIVIAL(debug) << "Mentions found: ";

            for (const auto& mention : msg.mentions) {
                BOOST_LOG_TRIVIAL(debug) << "@" << mention << " ";
            }
        }

        // Broadcast to all users
        nlohmann::json broadcastMsg = {
            {"type", "message"},
            {"id", msg.id},
            {"sender", msg.sender},
            {"content", msg.content},
            {"mentions", msg.mentions},
            {"timestamp", msg.timestamp}
        };

        BOOST_LOG_TRIVIAL(debug) << "Broadcasting message to all users";
        BroadcastMessage("system", broadcastMsg.dump());

        // send notifications to mentioned users
        if (!msg.mentions.empty()) {
            BOOST_LOG_TRIVIAL(debug) << "Sending notifications to mentioned users";
            NotifyMentionedUsers(msg);
        }
    }

    void ChatServerHandler::HandleDirectMessage(const std::string& sender, const nlohmann::json& msgData) {
        std::string recipient = msgData["recipient"];
        std::string content = msgData["content"];

        BOOST_LOG_TRIVIAL(debug) << "Sending direct message from" << sender << " to " << recipient;

        nlohmann::json dmMsg = {
            {"type", "direct_message"},
            {"sender", sender},
            {"content", content}
        };

        std::lock_guard<std::mutex> lock(chatServer.clientsMutex);
        if (auto it = chatServer.clients.find(recipient); it != chatServer.clients.end()) {
            try {
                auto& client = it->second;
                client.GetWebSocket()->text(true);
                client.GetWebSocket()->write(net::buffer(dmMsg.dump()));
                BOOST_LOG_TRIVIAL(debug) << "Direct message sent to " << recipient;

            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << "Error sending DM to " << recipient << ": " << e.what();
            }
        } else if (App::g_runDebug) {
            BOOST_LOG_TRIVIAL(info) << "Recipient" << recipient << " not found or offline";
        }
    }

    void ChatServerHandler::HandleEditMessage(const std::string& sender, const nlohmann::json& msgData) {
        std::string msgId = msgData["id"];
        std::string newContent = msgData["content"];

        BOOST_LOG_TRIVIAL(debug) << "Editing message " << msgId << " from " << sender;

        std::lock_guard<std::mutex> lock(chatServer.messagesMutex);
        auto& msg = chatServer.messages[msgId];

        if (msg.sender != sender) {
            BOOST_LOG_TRIVIAL(debug) << "Edit rejected: User " << sender << " is not the original sender";
            return;
        }

        msg.content = newContent;
        msg.mentions = ExtractMentions(msgData);
        msg.isEdited = true;

        BOOST_LOG_TRIVIAL(debug) << "Message edited successfully";

        nlohmann::json editMsg = {
            {"type", "message_edit"},
            {"id", msg.id},
            {"content", msg.content},
            {"mentions", msg.mentions}
        };

        BOOST_LOG_TRIVIAL(debug) << "Broadcasting edit to all users";
        BroadcastMessage("system", editMsg.dump());

        if (!msg.mentions.empty()) {
            BOOST_LOG_TRIVIAL(debug) << "Sending notifications for new mentions";
            NotifyMentionedUsers(msg);
        }
    }

    void ChatServerHandler::HandleDeleteMessage(const std::string& sender, const nlohmann::json& msgData) {
        std::string msgId = msgData["id"];
        BOOST_LOG_TRIVIAL(debug) << "Deleting message " << msgId << " from " << sender;

        std::lock_guard<std::mutex> lock(chatServer.messagesMutex);
        auto& msg = chatServer.messages[msgId];

        if (msg.sender != sender) {
            BOOST_LOG_TRIVIAL(warning) << "Delete rejected: User " << sender << " is not the original sender";
            return;
        }

        msg.isDeleted = true;
        msg.content = "[Message deleted]";
        msg.mentions.clear();

        BOOST_LOG_TRIVIAL(debug) << "Message deleted successfully";
        nlohmann::json deleteMsg = {
            {"type", "message_delete"},
            {"id", msg.id}
        };

        BOOST_LOG_TRIVIAL(debug) << "Broadcasting deletion to all users";
        BroadcastMessage("system", deleteMsg.dump());
    }

    void ChatServerHandler::NotifyMentionedUsers(const Message& msg) {
        std::lock_guard<std::mutex> lock(chatServer.clientsMutex);

        nlohmann::json notification = {
            {"type", "mention"},
            {"messageId", msg.id},
            {"sender", msg.sender},
            {"content", msg.content},
        };

        for (const auto& mention : msg.mentions) {
            BOOST_LOG_TRIVIAL(debug) << "Attempting to notify user: " << mention;

            if (auto it = chatServer.clients.find(mention); it != chatServer.clients.end()) {
                try {
                    auto& client = it->second;
                    client.GetWebSocket()->text(true);
                    client.GetWebSocket()->write(net::buffer(notification.dump()));
                    BOOST_LOG_TRIVIAL(debug) << "Notification sent to " << mention;
                } catch (const std::exception& e) {
                    BOOST_LOG_TRIVIAL(error) << "Error sending notification to " << mention << ": " << e.what();
                }
            } else {
                //TODO: do someting when mentioned users not found or offline
                BOOST_LOG_TRIVIAL(debug) << "Mentioned user " << mention << " not found or offline";
            }
        }
    }

    // Helper function to extract mentions from message
    std::vector<std::string> ChatServerHandler::ExtractMentions(const std::string& content) {
        std::vector<std::string> mentions;
        std::regex mention_pattern("@(\\w+)");

        auto words_begin = std::sregex_iterator(content.begin(), content.end(), mention_pattern);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            mentions.push_back(match[1].str());
        }
        return mentions;
    }
}