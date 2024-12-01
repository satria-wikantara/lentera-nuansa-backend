//
// Created by I Gede Panca Sutresna on 07/11/24.
//

#include <iostream>
#include <thread>
#include <yaml-cpp/yaml.h>

#include "../include/app.h"
#include "../include/chat_server.h"
#include "../include/chat_server_handler.h"
#include "../include/program_options.h"
#include "../include/global_config.h"

namespace App {
    void Run(const ProgramOptions &options) {
        if (options.command == "run") {
            try {
                net::io_context ioc;
                auto const address = net::ip::make_address(g_serverConfig.host);
                tcp::acceptor acceptor(ioc, {{address}, g_serverConfig.port});

                std::cout << "Websocket server running on port " << g_serverConfig.port << "..." << std::endl;

                auto chatServer = std::make_shared<App::ChatServer>();
                App::ChatServerHandler handler(chatServer);

                while (true) {
                    tcp::socket socket{ioc};
                    acceptor.accept(socket);

                    std::shared_ptr<websocket::stream<tcp::socket> > ws =
                            std::make_shared<websocket::stream<tcp::socket> >(std::move(socket));

                    std::thread{
                        [&handler, ws]() mutable {
                            handler.HandleWebSocketSession(ws);
                        }
                    }.detach();
                }
            } catch (const std::exception &e) {
                std::cerr << "Server error: " << e.what() << std::endl;
            }

            return;
        }

        std::cerr << "No valud command provided." << std::endl;
    }
}
