//
// Created by I Gede Panca Sutresna on 07/11/24.
//

#include <iostream>
#include <thread>

#include "../include/app.h"
#include "../include/chat_server.h"
#include "../include/chat_server_handler.h"
#include "../include/program_options.h"

namespace App {

    void Run(const ProgramOptions& options) {
        if (options.command == "run") {
            if (options.configFile.empty()) {
                std::cerr << "Non configuration file provided." << std::endl;
                return;
            }

            try {
                net::io_context ioc;
                tcp::acceptor acceptor(ioc, {{}, 9090});

                std::cout << "Websocket server running on port 9090..." << std::endl;

                while (true) {
                    tcp::socket socket{ioc};
                    acceptor.accept(socket);

                    std::shared_ptr<websocket::stream<tcp::socket>> ws =
                        std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));

                    std::thread{[ws]() mutable {
                        ChatServerHandler::HandleWebSocketSession(ws);
                    }}.detach();

                }
            } catch (const std::exception& e) {
                std::cerr << "Server error: " << e.what() << std::endl;
            }

            return;
        }

        std::cerr << "No valud command provided." << std::endl;
    }
}