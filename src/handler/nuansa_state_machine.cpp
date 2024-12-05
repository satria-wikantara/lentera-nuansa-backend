//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/pch/pch.h"

#include "nuansa/handler/websocket_client.h"
#include "nuansa/handler/websocket_handler.h"

#include "nuansa/handler/websocket_state_machine.h"
#include "nuansa/services/auth/auth_service.h"
#include "nuansa/services/user/user_service.h"
#include "nuansa/plugin/plugin_manager.h"

#include "nuansa/messages/message_types.h"

using namespace nuansa::messages;

namespace nuansa::handler {
    WebSocketStateMachine::WebSocketStateMachine(
        std::shared_ptr<WebSocketClient> client,
        std::shared_ptr<WebSocketServer> server)
        : client(client),
          websocketServer(server),
          state(ClientState::Initial) {
    }

    void WebSocketStateMachine::ProcessMessage(const nlohmann::json &msgData) {
        try {
            auto type = msgData["header"]["messageType"].get<std::string>();

            BOOST_LOG_TRIVIAL(debug) << "Message Type: " << type;

            // Handle authentication separately
            if (type == ToString(MessageType::Login) ||
                type == ToString(MessageType::Register)) {
                HandleAuthMessage(msgData);
                return;
            }

            // Process message based on current state
            switch (client->GetState()) {
                case ClientState::Initial:
                    HandleInitialState(msgData);
                    break;

                case ClientState::AwaitingAuth:
                    HandleAwaitingAuthState(msgData);
                    break;

                case ClientState::Authenticated:
                    HandleAuthenticatedState(msgData);
                    break;

                default:
                    BOOST_LOG_TRIVIAL(warning) << "Invalid state";
                    break;
            }
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error processing message: " << e.what();
            SendErrorMessage("Error processing message");
        }
    }

    void WebSocketStateMachine::HandleAuthMessage(const nlohmann::json &msgData) {
        auto type = msgData["header"]["messageType"].get<nuansa::messages::MessageType>();

        if (type == nuansa::messages::MessageType::Register) {
            HandleRegistration(msgData);
            TransitionTo(ClientState::AwaitingAuth);
        } else if (type == nuansa::messages::MessageType::Login) {
            if (HandleLogin(msgData)) {
                TransitionTo(ClientState::Authenticated);
                AddAuthenticatedClient();
            } else {
                TransitionTo(ClientState::AwaitingAuth);
            }
        }
    }

    void WebSocketStateMachine::HandleRegistration(const nlohmann::json &msgData) {
        try {
            // Extract header from the nested structure
            auto messageHeader = nuansa::messages::MessageHeader::FromJson(msgData["header"]);

            // Extract data from the nested structure
            auto username = msgData["payload"]["username"].get<std::string>();
            auto email = msgData["payload"]["email"].get<std::string>();
            auto password = msgData["payload"]["password"].get<std::string>();

            BOOST_LOG_TRIVIAL(debug) << "Processing registration request for user: " << username;

            // Create registration request
            nuansa::messages::RegisterRequest registrationRequest{
                messageHeader,
                username,
                email,
                password
            };

            // Get auth service instance and register
            auto &authService = nuansa::services::auth::AuthService::GetInstance();
            auto registrationResponse = authService.Register(registrationRequest);

            // Prepare response JSON with similar structure
            nlohmann::json response = {
                {
                    "header", {
                        {"version", "1.0"},
                        {"messageType", "register"},
                        {"messageId", messageHeader.messageId},
                        {"correlationId", messageHeader.correlationId},
                        {"timestamp", std::time(nullptr)}
                    }
                },
                {
                    "payload", {
                        {"success", registrationResponse.success},
                        {"message", registrationResponse.message},
                        {"token", registrationResponse.token} // Include token if registration successful
                    }
                }
            };

            if (registrationResponse.success) {
                BOOST_LOG_TRIVIAL(info) << "User registered successfully: " << username;
            } else {
                BOOST_LOG_TRIVIAL(warning) << "Registration failed for user: " << username;
            }
            SendMessage(response.dump());
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error during registration: " << e.what();

            nlohmann::json errorResponse = {
                {
                    "header", {
                        {"version", "1.0"},
                        {"messageType", "register"},
                        {"timestamp", std::time(nullptr)}
                    }
                },
                {
                    "payload", {
                        {"success", false},
                        {"message", "Internal server error during registration"}
                    }
                }
            };
            SendMessage(errorResponse.dump());
        }
    }

    bool WebSocketStateMachine::HandleLogin(const nlohmann::json &msgData) {
        try {
            auto username = msgData["username"].get<std::string>();
            auto password = msgData["password"].get<std::string>();

            BOOST_LOG_TRIVIAL(debug) << "Processing login request for user: " << username;

            // Create auth request
            nuansa::messages::AuthRequest authRequest{
                username,
                password
            };

            // Get auth service instance and authenticate
            auto &authService = nuansa::services::auth::AuthService::GetInstance();
            auto authResponse = authService.Authenticate(authRequest);

            // Prepare response JSON
            nlohmann::json response = {
                {"type", nuansa::messages::MessageType::Login},
                {"success", authResponse.success},
                {"message", authResponse.message}
            };

            if (authResponse.success) {
                BOOST_LOG_TRIVIAL(info) << "User authenticated successfully: " << username;

                // Update client state
                client->username = username;
                client->authToken = authResponse.token;
                client->authStatus = nuansa::messages::AuthStatus::Authenticated;

                // Add token to response
                response["token"] = authResponse.token;

                // Send success response
                SendMessage(response.dump());
                return true;
            } else {
                BOOST_LOG_TRIVIAL(warning) << "Authentication failed for user: " << username;

                // Update client state
                client->authStatus = nuansa::messages::AuthStatus::NotAuthenticated;
                client->authToken = std::nullopt;

                // Send failure response
                SendMessage(response.dump());
                return false;
            }
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error during login: " << e.what();

            // Send error response
            nlohmann::json errorResponse = {
                {"type", nuansa::messages::MessageType::Login},
                {"success", false},
                {"message", "Internal server error during login"}
            };
            SendMessage(errorResponse.dump());
            return false;
        }
    }

    // Helper method to send messages
    void WebSocketStateMachine::SendMessage(const std::string &message) {
        if (!client || !client->GetWebSocket()) {
            BOOST_LOG_TRIVIAL(error) << "Cannot send message - invalid client or websocket";
            return;
        }

        try {
            auto ws = client->GetWebSocket();
            ws->text(true);
            ws->write(net::buffer(message));
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error sending message: " << e.what();
        }
    }

    void WebSocketStateMachine::HandleInitialState(const nlohmann::json &msgData) {
        SendAuthRequiredMessage();
        TransitionTo(ClientState::AwaitingAuth);
    }

    void WebSocketStateMachine::HandleAwaitingAuthState(const nlohmann::json &msgData) {
        SendAuthRequiredMessage();
    }

    void WebSocketStateMachine::HandleAuthenticatedState(const nlohmann::json &msgData) {
        auto type = msgData["type"].get<nuansa::messages::MessageType>();

        switch (type) {
            case nuansa::messages::MessageType::Logout:
                HandleLogout();
                TransitionTo(ClientState::AwaitingAuth);
                break;

            case nuansa::messages::MessageType::New:
                HandleNewMessage(msgData);
                break;

            case nuansa::messages::MessageType::Edit:
                HandleEditMessage(msgData);
                break;

            case nuansa::messages::MessageType::Delete:
                HandleDeleteMessage(msgData);
                break;

            case nuansa::messages::MessageType::DirectMessage:
                HandleDirectMessage(msgData);
                break;

            default:
                BOOST_LOG_TRIVIAL(warning) << "Unhandled message type";
                break;
        }
    }

    void WebSocketStateMachine::TransitionTo(ClientState newState) {
        BOOST_LOG_TRIVIAL(debug) << "State transition: "
                            << static_cast<int>(state)
                            << " -> "
                            << static_cast<int>(newState);
        state = newState;
        client->SetState(newState);
    }

    void WebSocketStateMachine::HandleLogout() {
        // Implement logout logic
    }

    void WebSocketStateMachine::HandleNewMessage(const nlohmann::json &msg) {
        // Implement new message handling
    }

    void WebSocketStateMachine::SendErrorMessage(const std::string &error) {
        // Implement error message sending
    }

    void WebSocketStateMachine::HandleEditMessage(const nlohmann::json &msg) {
        // Implement edit message handling
    }

    void WebSocketStateMachine::HandleDeleteMessage(const nlohmann::json &msg) {
        // Implement delete message handling
    }

    void WebSocketStateMachine::HandleDirectMessage(const nlohmann::json &msg) {
        // Implement direct message handling
    }

    void WebSocketStateMachine::AddAuthenticatedClient() {
        // Implement client authentication logic
    }

    void WebSocketStateMachine::SendAuthRequiredMessage() {
        // Implement auth required message sending
    }
} // namespace nuansa::handler
