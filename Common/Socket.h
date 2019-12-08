#pragma once
#include <atomic>
#include <string_view>

// FIXME: This should be private
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
    ERR setTimeout(uint32_t timeout, int option);
    ERR init(std::string_view ip_str, uint16_t port, IPPROTO protocol);
    ERR connect(std::string_view ip_str, uint16_t port);
    ERR shutdown(int how);
    ERR close();

    // this is wrong!
    std::atomic_uint8_t status; // Count of opened streams (read/write) of the socket
};
