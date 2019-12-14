#pragma once
#include <atomic>

#include <Ws2tcpip.h>
#include <winsock2.h>

#include "Error.h"

class Socket {
private:
    SOCKET socket;

public:
    Socket()
        : socket{ INVALID_SOCKET }
        , status{ 0 }
    {
    }

    ~Socket()
    {
        shutdown(SD_BOTH);
        close();
    }

    SOCKET getSocket() const { return socket; };
    ClientError setTimeout(uint32_t timeout, int option);
    // PCSTR??????
    ClientError init(PCSTR ip_str, uint16_t port, IPPROTO protocol);
    ClientError connect(PCSTR ip_str, uint16_t port);
    ClientError shutdown(int how);
    ClientError close();

    // this is wrong!
    std::atomic_uint8_t status; // Count of opened streams (read/write) of the socket
};
