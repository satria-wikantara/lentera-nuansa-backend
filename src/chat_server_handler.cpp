
#include <nlohmann/json.hpp>
#include <iostream>
#include <regex>
#include <boost/beast/core.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>

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
            if (App::g_runDebug) {
                std::cout << "🔌 New websocket connection accepted" << std::endl;
            }

            beast::flat_buffer buffer;
            ws->read(buffer);
            std::string auth_message = beast::buffers_to_string(buffer.data());

            nlohmann::json auth_data = nlohmann::json::parse(auth_message);
            std::string username = auth_data["username"];

            if (App::g_runDebug) {
                std::cout << "🔑 User authentication: " << username << std::endl;
            }

            {
                std::lock_guard<std::mutex> lock(chatServer.clientsMutex);
                chatServer.clients[username] = ChatClient{username, ws};
                if (App::g_runDebug) {
                    std::cout << "✅ User " << username << " added to active clients" << std::endl;
                }
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
                    std::cerr << "❌ JSON parsing error: " << e.what() << std::endl;
                }
            }
        } catch (beast::system_error const& se) {
            if (se.code() != websocket::error::closed) {
                std::cerr << "❌ WebSocket error: " << se.code().message() << std::endl;
            }
        } catch (std::exception const& e) {
            std::cerr << "❌ Error: " << e.what() << std::endl;
        }

        // Handle disconnection
        std::string username;
        {
            std::lock_guard<std::mutex> lock(chatServer.clientsMutex);
            for (auto it = chatServer.clients.begin(); it != chatServer.clients.end(); ++it) {
                if (it->second.GetWebSocket().get() == ws.get()) {
                    username = it->first;
                    chatServer.clients.erase(it);
                    if (App::g_runDebug) {
                        std::cout << "User " << username << " disconnected" << std::endl;
                    }
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
                std::cerr << "Error broadcasting to " << username << ": " << e.what() << std::endl;
            }
        }
    }

    void ChatServerHandler::HandleMessage(const std::string& sender, const nlohmann::json& msgData) {

        if (App::g_runDebug) {
            std::cout << "📨 Received message from " << sender << ": " << msgData.dump(2) << std::endl;
        }

        try {
            MessageType type = msgData["type"].get<MessageType>();
            if (App::g_runDebug) {
                std::cout << "🔍 Processing message type: " << msgData["type"] << std::endl;
            }

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
            std::cerr << "❌ Error processing message: " << e.what() << std::endl;
        }
    }

    void ChatServerHandler::HandleNewMessage(const std::string& sender, const nlohmann::json& msgData) {
        if (App::g_runDebug) {
            std::cout << "📝 Creating new message from " << sender << std::endl;
        }

        boost::uuids::random_generator gen;
        Message msg {
            .id = boost::uuids::to_string(gen()),
            .sender = sender,
            .content = msgData["content"],
            .mentions = ExtractMentions(msgData["content"]),
            .timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())
        };

        if (App::g_runDebug) {
            std::cout << "🆔Generated message ID: " << msg.id << std::endl;
        }

        // Store message in history
        {
            std::lock_guard<std::mutex> lock(chatServer.messagesMutex);
            chatServer.messages[msg.id] = msg;
            if (App::g_runDebug) {
                std::cout << "💾Message stored in history" << std::endl;
            }
        }

        if (!msg.mentions.empty() && App::g_runDebug) {
            std::cout << "👥Mentions found: ";
            for (const auto& mention : msg.mentions) {
                std::cout << "@" << mention << " ";
            }
            std::cout << std::endl;
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

        if (App::g_runDebug) {
            std::cout << "📢Broadcasting message to all users" << std::endl;
        }
        BroadcastMessage("system", broadcastMsg.dump());

        // send notifications to mentioned users
        if (!msg.mentions.empty() && App::g_runDebug) {
            std::cout << "🔔Sending notifications to mentioned users" << std::endl;
        }
        NotifyMentionedUsers(msg);
    }

    void ChatServerHandler::HandleDirectMessage(const std::string& sender, const nlohmann::json& msgData) {
        std::string recipient = msgData["recipient"];
        std::string content = msgData["content"];

        if (App::g_runDebug) {
            std::cout << "📩 Sending direct message from" << sender << " to " << recipient << std::endl;
        }

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
                if (App::g_runDebug) {
                    std::cout << "✅ Direct message sent to " << recipient << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "❌ Error sending DM to " << recipient << ": " << e.what() << std::endl;
            }
        } else if (App::g_runDebug) {
            std::cout << "⚠️ Recipient" << recipient << " not found or offline" << std::endl;
        }
    }

    void ChatServerHandler::HandleEditMessage(const std::string& sender, const nlohmann::json& msgData) {
        std::string msgId = msgData["id"];
        std::string newContent = msgData["content"];

        if (App::g_runDebug) {
            std::cout << "📝Editing message " << msgId << " from " << sender << std::endl;
        }

        std::lock_guard<std::mutex> lock(chatServer.messagesMutex);
        auto& msg = chatServer.messages[msgId];

        if (msg.sender != sender) {
            if (App::g_runDebug) {
                std::cout << "⛔Edit rejected: User " << sender << " is not the original sender " << std::endl;
            }
            return;
        }

        msg.content = newContent;
        msg.mentions = ExtractMentions(msgData);
        msg.isEdited = true;

        if (App::g_runDebug) {
            std::cout << "✅ Message edited successfully" << std::endl;
        }

        nlohmann::json editMsg = {
            {"type", "message_edit"},
            {"id", msg.id},
            {"content", msg.content},
            {"mentions", msg.mentions}
        };

        if (App::g_runDebug) {
            std::cout << "📢 Broadcasting edit to all users" << std::endl;
        }
        BroadcastMessage("system", editMsg.dump());

        if (!msg.mentions.empty() && App::g_runDebug) {
            std::cout << "🔔 Sending notifications for new mentions" << std::endl;
        }
        NotifyMentionedUsers(msg);
    }

    void ChatServerHandler::HandleDeleteMessage(const std::string& sender, const nlohmann::json& msgData) {
        std::string msgId = msgData["id"];
        if (App::g_runDebug) {
            std::cout << "🗑️Deleting message " << msgId << " from " << sender << std::endl;
        }

        std::lock_guard<std::mutex> lock(chatServer.messagesMutex);
        auto& msg = chatServer.messages[msgId];

        if (msg.sender != sender) {
            if (App::g_runDebug) {
                std::cout << "⛔ Delete rejected: User " << sender << " is not the original sender" << std::endl;
            }
            return;
        }

        msg.isDeleted = true;
        msg.content = "[Message deleted]";
        msg.mentions.clear();

        if (App::g_runDebug) {
            std::cout << "✅ Message deleted successfully" << std::endl;
        }

        nlohmann::json deleteMsg = {
            {"type", "message_delete"},
            {"id", msg.id}
        };

        if (App::g_runDebug) {
            std::cout << "📢 Broadcasting deletion to all users" << std::endl;
        }
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
            if (App::g_runDebug) {
                std::cout << "🔔 Attempting to notify user: " << mention << std::endl;
            }

            if (auto it = chatServer.clients.find(mention); it != chatServer.clients.end()) {
                try {
                    auto& client = it->second;
                    client.GetWebSocket()->text(true);
                    client.GetWebSocket()->write(net::buffer(notification.dump()));
                    if (App::g_runDebug) {
                        std::cout << "✅ Notification sent to " << mention << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "❌ Error sending notification to " << mention << ": " << e.what() << std::endl;
                }
            } else if (App::g_runDebug) {
                std::cout << "⚠️ Mentioned user " << mention << " not found or offline" << std::endl;
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