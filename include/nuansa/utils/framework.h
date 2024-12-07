#ifndef NUANSA_UTILS_FRAMEWORK_H
#define NUANSA_UTILS_FRAMEWORK_H

#ifdef _WIN32
    #include <Windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif


#endif /* NUANSA_UTILS_FRAMEWORK_H */
