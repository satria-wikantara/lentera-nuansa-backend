//
// Created by I Gede Panca Sutresna on 09/11/24.
//

#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include <string>

namespace App {

    struct ServerConfig {
        std::string host;
        unsigned short port;
    };

    extern bool g_runDebug; // Global debug flag
    extern ServerConfig g_serverConfig;
}

#endif //GLOBAL_CONFIG_H
