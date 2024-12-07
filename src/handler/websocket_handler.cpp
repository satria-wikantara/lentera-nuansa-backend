//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/pch/pch.h"
#include "nuansa/handler/websocket_handler.h"
#include "nuansa/handler/websocket_state_machine.h"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;

namespace nuansa::handler {
    WebSocketHandler::WebSocketHandler(const std::shared_ptr<WebSocketServer> &server)
        : websocketServer(server) {
    }

    void WebSocketHandler::HandleSession(std::shared_ptr<websocket::stream<tcp::socket> > ws) {
        std::shared_ptr<WebSocketClient> client;
        try {
            LOG_DEBUG << "Starting new WebSocket session";

            // Set timeout options
            LOG_DEBUG << "Setting timeout options";
            ws->set_option(websocket::stream_base::timeout::suggested(
                beast::role_type::server));

            // Set a decorator to handle the WebSocket handshake
            LOG_DEBUG << "Setting WebSocket decorator";
            ws->set_option(websocket::stream_base::decorator(
                [](websocket::response_type &res) {
                    res.set(http::field::server, "Kudeta WebSocket Server");
                    res.set(http::field::access_control_allow_origin, "*");
                }));

            LOG_DEBUG << "Generating client UUID";
            boost::uuids::random_generator generator;
            boost::uuids::uuid uuid = generator();
            std::string clientId = boost::uuids::to_string(uuid);
            LOG_DEBUG << "Generated client ID: " << clientId;

            LOG_DEBUG << "Creating WebSocket client and state machine";
            client = std::make_shared<WebSocketClient>(clientId, ws);
            auto stateMachine = std::make_shared<WebSocketStateMachine>(client, websocketServer);

            beast::flat_buffer buffer;
            LOG_DEBUG << "Entering message processing loop";
            while (stateMachine->GetCurrentState() != ClientState::Disconnected) {
                boost::system::error_code ec;
                buffer.consume(buffer.size());

                LOG_DEBUG << "Waiting for next message";
                // Add timeout for read operations
                ws->read(buffer, ec);

                if (ec) {
                    if (ec != websocket::error::closed) {
                        LOG_ERROR << "Read error: " << ec.message();
                    }
                    LOG_DEBUG << "Breaking message loop due to error: " << ec.message();
                    break;
                }

                std::string message = beast::buffers_to_string(buffer.data());
                LOG_DEBUG << "Received message: " << message;

                try {
                    LOG_DEBUG << "Parsing message JSON";
                    nlohmann::json msgData = nlohmann::json::parse(message);
                    LOG_DEBUG << "Processing message through state machine";
                    stateMachine->ProcessMessage(msgData);
                } catch (const nlohmann::json::exception &e) {
                    LOG_ERROR << "JSON parsing error: " << e.what();
                    LOG_DEBUG << "Sending error message to client";
                    SendErrorMessage(client, "Invalid message format");
                }
            }

            // Perform clean shutdown
            LOG_DEBUG << "Performing clean WebSocket shutdown";
            ws->close(websocket::close_code::normal);
        } catch (const beast::system_error &se) {
            if (se.code() != websocket::error::closed) {
                LOG_ERROR << "WebSocket error: " << se.code().message();
            }
            LOG_DEBUG << "Caught beast::system_error: " << se.what();
        } catch (const std::exception &e) {
            LOG_ERROR << "Session error: " << e.what();
            LOG_DEBUG << "Caught std::exception: " << e.what();
        }
        // Ensure client cleanup happens
        if (client) {
            LOG_DEBUG << "Cleaning up client connection";
            HandleClientDisconnection(client);
        }
    }

    void WebSocketHandler::SendMessage(const std::shared_ptr<WebSocketClient> &client,
                                       const std::string &message) {
        if (!client || !client->GetWebSocket()) {
            LOG_ERROR << "Invalid client";
            return;
        }

        try {
            const auto ws = client->GetWebSocket();
            ws->text(true);
            ws->write(net::buffer(message));
        } catch (const std::exception &e) {
            LOG_ERROR << "Error sending message: " << e.what();
        }
    }

    // TODO: Implement this
    void WebSocketHandler::SendMessage(std::shared_ptr<WebSocketClient> client, const nlohmann::json &jsonMessage) {
    }

    void WebSocketHandler::SendErrorMessage(const std::shared_ptr<WebSocketClient> &client,
                                            const std::string &errorMessage,
                                            const std::string &errorCode) {
        const nlohmann::json errorJson = {
            {"type", "error"},
            {"code", errorCode},
            {"message", errorMessage}
        };
        SendMessage(client, errorJson.dump());
    }

    void WebSocketHandler::HandleClientDisconnection(const std::shared_ptr<WebSocketClient> &client) const {
        if (!client) return;

        // Find and remove the client from the clients map
        if (const auto it = websocketServer->clients.find(client->username); it != websocketServer->clients.end()) {
            websocketServer->clients.erase(it);

            LOG_INFO << "Client disconnected: " << client->username;

            // Broadcast disconnection message
            const nlohmann::json disconnectMsg = {
                {"type", "system"},
                {"content", client->username + " has disconnected"}
            };

            BroadcastMessage("system", disconnectMsg.dump());
        }
    }

    void WebSocketHandler::BroadcastMessage(const std::string &sender, const std::string &message) const {
        const nlohmann::json broadcastMsg = {
            {"type", "broadcast"},
            {"sender", sender},
            {"content", message}
        };

        std::string msgStr = broadcastMsg.dump();

        for (auto &[username, client]: websocketServer->clients) {
            try {
                if (const auto ws = client.GetWebSocket()) {
                    ws->text(true);
                    ws->write(net::buffer(msgStr));
                    LOG_DEBUG << "Broadcast message sent to " << username;
                }
            } catch (const std::exception &e) {
                LOG_ERROR << "Error broadcasting to " << username << ": " << e.what();
            }
        }
    }

    void WebSocketHandler::NotifyMentionedUsers(const nuansa::messages::Message &msg) const {
        const nlohmann::json notification = {
            {"type", "mention"},
            {"messageId", msg.id},
            {"sender", msg.sender},
            {"content", msg.content}
        };

        for (const auto &mention: msg.mentions) {
            LOG_DEBUG << "Attempting to notify user: " << mention;

            if (auto it = websocketServer->clients.find(mention);
                it != websocketServer->clients.end()) {
                try {
                    auto &client = it->second;
                    if (const auto ws = client.GetWebSocket()) {
                        ws->text(true);
                        ws->write(net::buffer(notification.dump()));
                        LOG_DEBUG << "Notification sent to " << mention;
                    }
                } catch (const std::exception &e) {
                    LOG_ERROR << "Error sending notification to "
                                           << mention << ": " << e.what();
                }
            } else {
                LOG_DEBUG << "Mentioned user " << mention
                                       << " not found or offline";
            }
        }
    }

    std::vector<std::string> WebSocketHandler::ExtractMentions(const std::string &content) {
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

    void WebSocketHandler::SendAuthRequiredMessage(const std::shared_ptr<WebSocketClient> &client) {
        const nlohmann::json response = {
            {"type", nuansa::messages::MessageType::AuthRequired},
            {"message", "Authentication required"}
        };
        SendMessage(client, response.dump());
    }

    void WebSocketHandler::SendSystemMessage(const std::shared_ptr<WebSocketClient> &client,
                                             const std::string &message) {
        const nlohmann::json systemMsg = {
            {"type", "system"},
            {"content", message}
        };
        SendMessage(client, systemMsg.dump());
    }

    void WebSocketHandler::SendAckMessage(const std::shared_ptr<WebSocketClient> &client,
                                          const std::string &messageId,
                                          bool success,
                                          const std::string &details) {
        nlohmann::json ackMsg = {
            {"type", "ack"},
            {"messageId", messageId},
            {"success", success}
        };

        if (!details.empty()) {
            ackMsg["details"] = details;
        }

        SendMessage(client, ackMsg.dump());
    }

    void WebSocketHandler::ValidateMessageFormat(const nlohmann::json &msgData) {
        // Check required fields based on message type

        switch (msgData["type"].get<nuansa::messages::MessageType>()) {
            case nuansa::messages::MessageType::New:
                if (!msgData.contains("content")) {
                    throw std::invalid_argument("New message must contain 'content' field");
                }
                break;

            case nuansa::messages::MessageType::Edit:
                if (!msgData.contains("id") || !msgData.contains("content")) {
                    throw std::invalid_argument("Edit message must contain 'id' and 'content' fields");
                }
                break;

            case nuansa::messages::MessageType::Delete:
                if (!msgData.contains("id")) {
                    throw std::invalid_argument("Delete message must contain 'id' field");
                }
                break;

            case nuansa::messages::MessageType::DirectMessage:
                if (!msgData.contains("recipient") || !msgData.contains("content")) {
                    throw std::invalid_argument("Direct message must contain 'recipient' and 'content' fields");
                }
                break;

            default:
                break;
        }
    }

    bool WebSocketHandler::IsUserOnline(const std::string &username) const {
        return websocketServer->clients.contains(username);
    }

    std::size_t WebSocketHandler::GetOnlineUserCount() const {
        return websocketServer->clients.size();
    }

    std::vector<std::string> WebSocketHandler::GetOnlineUsers() const {
        std::vector<std::string> onlineUsers;
        onlineUsers.reserve(websocketServer->clients.size());

        for (const auto &username: websocketServer->clients | std::views::keys) {
            onlineUsers.push_back(username);
        }

        return onlineUsers;
    }

    void WebSocketHandler::SendOnlineUsersList(const std::shared_ptr<WebSocketClient> &client) const {
        const nlohmann::json userListMsg = {
            {"type", "userList"},
            {"users", GetOnlineUsers()},
            {"count", GetOnlineUserCount()}
        };

        SendMessage(client, userListMsg.dump());
    }

    void WebSocketHandler::SendTypingNotification(const std::string &username,
                                                  bool isTyping,
                                                  const std::string &recipient) const {
        const nlohmann::json typingMsg = {
            {"type", "typing"},
            {"username", username},
            {"isTyping", isTyping}
        };

        if (recipient.empty()) {
            // Broadcast to all users
            BroadcastMessage("system", typingMsg.dump());
        } else {
            // Send to specific recipient
            if (const auto it = websocketServer->clients.find(recipient);
                it != websocketServer->clients.end()) {
                SendMessage(std::make_shared<WebSocketClient>(it->second), typingMsg.dump());
            }
        }
    }
}
