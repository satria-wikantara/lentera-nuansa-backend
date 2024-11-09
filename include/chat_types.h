#ifndef APP_CHAT_TYPES_H
#define APP_CHAT_TYPES_H

#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace App {
    struct ChatClient {
        std::string username;
        std::shared_ptr<websocket::stream<tcp::socket>> ws;
    };

    struct ChatServer {
        std::map<std::string, ChatClient> clients;
        std::mutex clientsMutex;
    };
}

#endif /* APP_CHAT_TYPES_H */
