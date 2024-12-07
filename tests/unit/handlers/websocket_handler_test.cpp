//
// Created by I Gede Panca Sutresna on 01/12/24.
//
#include "nuansa/utils/pch.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>


#include "nuansa/handler/websocket_handler.h"
#include "nuansa/handler/websocket_server.h"
#include "nuansa/messages/message_types.h"
#include "nuansa/handler/websocket_state_machine.h"

using namespace testing;
using json = nlohmann::json;

namespace App {
	// Mock WebSocket class
	// class MockWebSocket : public websocket::stream<tcp::socket> {
	// public:
	//     MOCK_METHOD(void, write, (const boost::beast::flat_buffer&));
	//     MOCK_METHOD(void, text, (bool));
	// };

	// class WebSocketHandlerTest : public Test {
	// protected:
	//     void SetUp() override {
	//         mockServer = std::make_shared<nuansa::handler::WebSocketServer>();
	//         handler = std::make_unique<nuansa::handler::WebSocketHandler>(mockServer);
	//         stateMachine = std::make_unique<nuansa::handler::WebSocketStateMachine>(handler, mockServer);
	//     }

	//     void TearDown() override {
	//         handler.reset();
	//         stateMachine.reset();
	//         mockServer.reset();
	//     }

	//     std::shared_ptr<nuansa::handler::WebSocketServer> mockServer;
	//     std::unique_ptr<nuansa::handler::WebSocketHandler> handler;
	//     std::unique_ptr<nuansa::handler::WebSocketStateMachine> stateMachine;
	// };

	// TEST_F(WebSocketHandlerTest, ExtractMentionsTest) {
	//     std::string content = "Hello @user1 and @user2! How are you @user3?";
	//     auto mentions = handler->ExtractMentions(content);

	//     ASSERT_EQ(mentions.size(), 3);
	//     EXPECT_EQ(mentions[0], "user1");
	//     EXPECT_EQ(mentions[1], "user2");
	//     EXPECT_EQ(mentions[2], "user3");
	// }

	// TEST_F(WebSocketHandlerTest, ExtractMentionsEmptyTest) {
	//     std::string content = "Hello everyone! No mentions here.";
	//     auto mentions = handler->ExtractMentions(content);

	//     EXPECT_TRUE(mentions.empty());
	// }

	// TEST_F(WebSocketHandlerTest, HandleNewMessageTest) {
	//     json msgData = {
	//         {"type", "new"},
	//         {"content", "Hello @user1!"}
	//     };

	//     stateMachine->HandleNewMessage(std::make_shared<nuansa::handler::WebSocketClient>("sender"), msgData);

	//     EXPECT_FALSE(chatServer.GetMessages().empty());

	//     // Get the first message
	//     const auto& msg = chatServer.GetMessages().begin()->second;
	//     EXPECT_EQ(msg.sender, "sender");
	//     EXPECT_EQ(msg.content, "Hello @user1!");
	//     EXPECT_EQ(msg.mentions.size(), 1);
	//     EXPECT_EQ(msg.mentions[0], "user1");
	// }

	// TEST_F(WebSocketHandlerTest, HandleEditMessageTest) {
	//     // First create a message
	//     std::string msgId = "test-id";
	//     nuansa::messages::Message originalMsg{
	//         .id = msgId,
	//         .sender = "sender",
	//         .content = "Original content",
	//         .mentions = {},
	//         .timestamp = std::time(nullptr)
	//     };

	//     auto& chatServer = handler->GetInstance();
	//     chatServer.SetMessage(originalMsg);

	//     // Now edit the message
	//     json editData = {
	//         {"type", "edit"},
	//         {"id", msgId},
	//         {"content", "Edited content @user1"}
	//     };

	//     handler->HandleEditMessage(std::make_shared<nuansa::handler::WebSocketClient>("sender"), editData);

	//     ASSERT_TRUE(chatServer.GetMessages().contains(msgId));
	//     const auto& editedMsg = chatServer.GetMessage(msgId);
	//     EXPECT_EQ(editedMsg->content, "Edited content @user1");
	//     EXPECT_TRUE(editedMsg->isEdited);
	//     EXPECT_EQ(editedMsg->mentions.size(), 1);
	//     EXPECT_EQ(editedMsg->mentions[0], "user1");
	// }

	// TEST_F(WebSocketHandlerTest, HandleDeleteMessageTest) {
	//     // First create a message
	//     std::string msgId = "test-id";
	//     nuansa::messages::Message originalMsg{
	//         .id = msgId,
	//         .sender = "sender",
	//         .content = "Original content",
	//         .mentions = {},
	//         .timestamp = std::time(nullptr)
	//     };

	//     auto& chatServer = handler->GetInstance();
	//     chatServer.SetMessage(originalMsg);

	//     // Now delete the message
	//     json deleteData = {
	//         {"type", "delete"},
	//         {"id", msgId}
	//     };

	//     handler->HandleDeleteMessage(std::make_shared<nuansa::handler::WebSocketClient>("sender"), deleteData);

	//     ASSERT_TRUE(chatServer.GetMessages().contains(msgId));
	//     const auto& deletedMsg = chatServer.GetMessage(msgId);
	//     EXPECT_EQ(deletedMsg->content, "[Message deleted]");
	//     EXPECT_TRUE(deletedMsg->isDeleted);
	//     EXPECT_TRUE(deletedMsg->mentions.empty());
	// }

	// TEST_F(WebSocketHandlerTest, UnauthorizedEditTest) {
	//     std::string msgId = "test-id";
	//     nuansa::messages::Message originalMsg{
	//         .id = msgId,
	//         .sender = "original_sender",
	//         .content = "Original content",
	//         .mentions = {},
	//         .timestamp = std::time(nullptr)
	//     };

	//     auto& chatServer = handler->GetInstance();
	//     chatServer.SetMessage(originalMsg);

	//     json editData = {
	//         {"type", "edit"},
	//         {"id", msgId},
	//         {"content", "Unauthorized edit"}
	//     };

	//     handler->HandleEditMessage(std::make_shared<nuansa::handler::WebSocketClient>("different_sender"), editData);

	//     ASSERT_TRUE(chatServer.GetMessages().contains(msgId));
	//     const auto& msg = chatServer.GetMessage(msgId);
	//     EXPECT_EQ(msg->content, "Original content");
	//     EXPECT_FALSE(msg->isEdited);
	// }

	// TEST_F(WebSocketHandlerTest, HandleNewMessageWithMultipleMentionsTest) {
	//     json msgData = {
	//         {"type", "new"},
	//         {"content", "Hello @user1 and @user2! CC: @user3"}
	//     };

	//     handler->HandleNewMessage(std::make_shared<nuansa::handler::WebSocketClient>("sender"), msgData);

	//     auto& chatServer = handler->GetInstance();
	//     ASSERT_FALSE(chatServer.GetMessages().empty());

	//     const auto& msg = chatServer.GetMessages().begin()->second;
	//     EXPECT_EQ(msg.sender, "sender");
	//     EXPECT_EQ(msg.content, "Hello @user1 and @user2! CC: @user3");
	//     EXPECT_EQ(msg.mentions.size(), 3);
	//     EXPECT_THAT(msg.mentions, UnorderedElementsAre("user1", "user2", "user3"));
	// }

	// TEST_F(WebSocketHandlerTest, UnauthorizedDeleteTest) {
	//     std::string msgId = "test-id";
	//     nuansa::messages::Message originalMsg{
	//         .id = msgId,
	//         .sender = "original_sender",
	//         .content = "Original content",
	//         .mentions = {},
	//         .timestamp = std::time(nullptr)
	//     };

	//     auto& chatServer = handler->GetInstance();
	//     chatServer.SetMessage(originalMsg);

	//     json deleteData = {
	//         {"type", "delete"},
	//         {"id", msgId}
	//     };

	//     handler->HandleDeleteMessage(std::make_shared<nuansa::handler::WebSocketClient>("different_sender"), deleteData);

	//     ASSERT_TRUE(chatServer.GetMessages().contains(msgId));
	//     const auto& msg = chatServer.GetMessage(msgId);
	//     EXPECT_EQ(msg->content, "Original content");
	//     EXPECT_FALSE(msg->isDeleted);
	// }
} // namespace App
