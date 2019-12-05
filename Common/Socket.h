#pragma once
#include <atomic>

#include <winsock2.h>
#include <Ws2tcpip.h>

#include "Error.h"

class Socket {
public:
    Socket() {
        socket = INVALID_SOCKET;
        status = 0;
    };
    ~Socket() {
        shutdown(SD_BOTH);
        close();
    };
    SOCKET getSocket() { return socket; };

    ERR setTimeout(uint32_t timeout, int option);
    ERR init(PCSTR IP, uint16_t port, IPPROTO protocol);
    ERR connect(PCSTR IP, uint16_t port);
    ERR shutdown(int how);
    ERR close();

    std::atomic_uint8_t status; // Count of opened streams (read/write) of the socket
private:
    SOCKET socket;
};