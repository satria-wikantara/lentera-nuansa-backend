#include "nuansa/pch/pch.h"

#include "nuansa/core/app.h"
#include "nuansa/handler/websocket_server.h"
#include "nuansa/handler/websocket_handler.h"
#include "nuansa/utils/program_options.h"
#include "nuansa/config/config.h"
#include "nuansa/database/db_connection_pool.h"
#include "nuansa/utils/errors/database_error.h"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;


namespace nuansa::core {
    bool Initialize(const std::string &configPath) {
        return CheckForRootUser() &&
               InitializeConfig(configPath) &&
               InitializeLogging() &&
               InitializeDatabase();
    }

    bool CheckForRootUser() {
        if (getuid() == 0) {
            std::cerr << "Warning: running as root is not recommended. Please use a non-root user." << std::endl;
            return false;
        }

        return true;
    }

    bool InitializeConfig(const std::string &configPath) {
        try {
            // Initialize configurations
            auto &config = nuansa::config::GetConfig();
            config.Initialize(configPath);
        } catch (const std::exception &e) {
            throw std::runtime_error("Failed to load config file: " + std::string(e.what()));
        }

        return true;
    }

    bool InitializeLogging() {
        auto &config = nuansa::config::GetConfig();
        std::filesystem::create_directories(std::filesystem::path(config.GetServerConfig().logPath).parent_path());

        boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

        // Only show debug messages when g_runDebug is true
        boost::log::core::get()->set_filter(
            boost::log::trivial::severity >= (config.GetServerConfig().logLevel == "debug"
                                                  ? boost::log::trivial::debug
                                                  : boost::log::trivial::info)
        );

        // Define logging format
        boost::log::add_console_log(
            std::cout,
            boost::log::keywords::format = "[%TimeStamp%] [%Severity%] %Message%"
        );

        // Add common attributes
        boost::log::add_common_attributes();

        return true;
    }

    bool InitializeDatabase() {
        auto &config = nuansa::config::GetConfig();

        try {
            nuansa::database::ConnectionPool::GetInstance().Initialize(
                config.GetDatabaseConfig().connection_string,
                config.GetDatabaseConfig().pool_size
            );
        } catch (const nuansa::utils::errors::DatabaseCreateConnectionError &e) {
            std::cerr << "Database connection pool initialization error: " << e.what() << std::endl;
            return false;
        }

        return true;
    }

    void Run(const utils::ProgramOptions &options) {
        BOOST_LOG_TRIVIAL(debug) << "Starting Run() with command: " << options.GetCommand();

        // If any of the checks fail, return an error
        if (!nuansa::core::Initialize(options.GetConfigFilePath().string())) {
            BOOST_LOG_TRIVIAL(error) << "Failed to initialize core components";
            return;
        }

        if (options.GetCommand() == "run") {
            try {
                BOOST_LOG_TRIVIAL(debug) << "Initializing io_context";
                net::io_context ioc;
                auto &serverConfig = nuansa::config::GetConfig().GetServerConfig();

                auto const address = net::ip::make_address(serverConfig.host);
                tcp::acceptor acceptor(ioc, {{address}, serverConfig.port});

                BOOST_LOG_TRIVIAL(info) << "WebSocket server running on port " << serverConfig.port;

                auto websocketServer = std::make_shared<nuansa::handler::WebSocketServer>();
                auto handler = std::make_shared<nuansa::handler::WebSocketHandler>(websocketServer);

                // Start accepting connections asynchronously
                auto DoAccept = [&acceptor, &ioc, handler](auto &&self) -> void {
                    BOOST_LOG_TRIVIAL(debug) << "Starting async accept";
                    acceptor.async_accept(
                        [handler, &acceptor, self=std::forward<decltype(self)>(self)]
                (boost::system::error_code ec, tcp::socket socket) {
                            if (!ec) {
                                BOOST_LOG_TRIVIAL(debug) << "New connection accepted";
                                // Create WebSocket stream
                                auto ws = std::make_shared<websocket::stream<tcp::socket> >(std::move(socket));

                                // Accept WebSocket handshake asynchronously
                                ws->async_accept(
                                    [handler, ws](boost::system::error_code ec) {
                                        if (!ec) {
                                            BOOST_LOG_TRIVIAL(debug) << "WebSocket handshake successful";
                                            // Set suggested timeout options
                                            ws->set_option(websocket::stream_base::timeout::suggested(
                                                beast::role_type::server));

                                            // Set the decorator for the handshake response
                                            ws->set_option(websocket::stream_base::decorator(
                                                [](websocket::response_type &res) {
                                                    res.set(http::field::server, "Nuansa WebSocket Server");
                                                    res.set(http::field::access_control_allow_origin, "*");
                                                }));

                                            // Start the session in a separate thread to avoid blocking
                                            std::thread([handler, ws]() {
                                                try {
                                                    BOOST_LOG_TRIVIAL(debug) << "Starting new session handler thread";
                                                    handler->HandleSession(ws);
                                                } catch (const std::exception &e) {
                                                    BOOST_LOG_TRIVIAL(error) << "Session handling error: " << e.what();
                                                }
                                                // Let the WebSocket close naturally through RAII
                                            }).detach();
                                        } else {
                                            BOOST_LOG_TRIVIAL(error) << "WebSocket accept error: " << ec.message();
                                        }
                                    });
                            } else {
                                BOOST_LOG_TRIVIAL(error)
                                    << "Accept error: " << ec.message();
                            }

                            // Continue accepting
                            self(self);
                        });
                };

                // Start the accept loop (recursive lambda)
                BOOST_LOG_TRIVIAL(debug) << "Starting accept loop";
                DoAccept(DoAccept);

                // Run the io_context
                std::vector<std::thread> threads;
                auto thread_count = std::thread::hardware_concurrency();
                threads.reserve(thread_count);

                BOOST_LOG_TRIVIAL(debug) << "Creating " << thread_count << " IO threads";
                // Create a pool of threads to run the io_context
                for (auto i = 0u; i < thread_count; ++i) {
                    threads.emplace_back([&ioc] {
                        try {
                            BOOST_LOG_TRIVIAL(debug) << "IO thread " << std::this_thread::get_id() << " starting";
                            ioc.run();
                        } catch (const std::exception &e) {
                            BOOST_LOG_TRIVIAL(error)
                                << "IO context error: " << e.what();
                        }
                    });
                }

                // Wait for all threads to complete
                BOOST_LOG_TRIVIAL(debug) << "Waiting for IO threads to complete";
                for (auto &thread: threads) {
                    thread.join();
                }
            } catch (const std::exception &e) {
                BOOST_LOG_TRIVIAL(error) << "Server error: " << e.what();
            }
            return;
        }

        BOOST_LOG_TRIVIAL(error) << "No valid command provided.";
    }
}
