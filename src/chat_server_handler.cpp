#include "../include/chat_server_handler.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <boost/beast/core.hpp>

namespace App {
    ChatServer ChatServerHandler::chatServer;

    ChatServer& ChatServerHandler::GetInstance() {
        return chatServer;
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

    void ChatServerHandler::HandleWebSocketSession(std::shared_ptr<websocket::stream<tcp::socket>> ws) {
        try {
            ws->accept();

            beast::flat_buffer buffer;
            ws->read(buffer);
            std::string auth_message = beast::buffers_to_string(buffer.data());

            nlohmann::json auth_data = nlohmann::json::parse(auth_message);
            std::string username = auth_data["username"];

            {
                std::lock_guard<std::mutex> lock(chatServer.clientsMutex);
                chatServer.clients[username] = ChatClient{username, ws};
            }

            BroadcastMessage("system", username + " has joined the chat");

            while (true) {
                buffer.consume(buffer.size());
                ws->read(buffer);
                std::string message = beast::buffers_to_string(buffer.data());

                try {
                    nlohmann::json msg_data = nlohmann::json::parse(message);

                    if (msg_data["type"] == "direct_message") {
                        std::string target = msg_data["target"];
                        std::string content = msg_data["content"];

                        std::lock_guard<std::mutex> lock(chatServer.clientsMutex);
                        if (auto it = chatServer.clients.find(target); it != chatServer.clients.end()) {
                            nlohmann::json dm = {
                                {"type", "direct_message"},
                                {"sender", username},
                                {"content", content}
                            };
                            it->second.ws->text(true);
                            it->second.ws->write(net::buffer(dm.dump()));
                        }
                    } else {
                        BroadcastMessage(username, msg_data["content"]);
                    }
                } catch (const nlohmann::json::exception& e) {
                    std::cerr << "JSON parsing error: " << e.what() << std::endl;
                }
            }
        } catch (beast::system_error const& se) {
            if (se.code() != websocket::error::closed) {
                std::cerr << "WebSocket error: " << se.code().message() << std::endl;
            }
        } catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }

        std::string username;
        {
            std::lock_guard<std::mutex> lock(chatServer.clientsMutex);
            for (auto it = chatServer.clients.begin(); it != chatServer.clients.end(); ++it) {
                if (it->second.ws.get() == ws.get()) {
                    username = it->first;
                    chatServer.clients.erase(it);
                    break;
                }
            }
        }

        if (!username.empty()) {
            BroadcastMessage("system", username + " has left the chat");
        }
    }
}