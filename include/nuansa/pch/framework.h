//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_PCH_FRAMEWORK_H
#define NUANSA_PCH_FRAMEWORK_H

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#endif //NUANSA_PCH_FRAMEWORK_H