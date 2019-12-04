#pragma once
#include <atomic>
#include <winsock2.h>
#include <Ws2tcpip.h>

#include "Constraints.h"
#include "Error.h"
#include "Log.h"

class Socket {
public:
    Socket() {
        socket = INVALID_SOCKET;
        status = 0;
    }
    ~Socket() {
        shutdown(SD_BOTH);
        close();
    }
    SOCKET getSocket() { return socket; }
    ERR setTimeout(uint32_t timeout, int option) {
        if (socket == INVALID_SOCKET) return E_UNKNOWN;

        uint32_t value = timeout * 1000;

        int err = setsockopt(socket, SOL_SOCKET, option, (char *)&value, sizeof(value));
        if (err == SOCKET_ERROR)
            return W_SET_TIMEOUT;

        return E_OK;
    }
    ERR init(PCSTR IP, uint16_t port, IPPROTO protocol) {
        int sock_type;

        switch (protocol) {
        case IPPROTO_TCP:
            sock_type = SOCK_STREAM;
            break;
        case IPPROTO_UDP:
            sock_type = SOCK_DGRAM;
            break;
        default:
            // Unexpected protocol
            return E_UNKNOWN_PROTOCOL;
        }

        socket = ::socket(AF_INET, sock_type, protocol);
        if (socket == INVALID_SOCKET)
            return E_CREATE_SOCKET;

        status = 2;

        return E_OK;
    }
    ERR connect(PCSTR IP, uint16_t port) {
        if (socket == INVALID_SOCKET) return E_UNKNOWN;

        sockaddr_in serverAddr{
            AF_INET,
            htons(port)
        };

        //socketDesc.sin_family = AF_INET;
        //socketDesc.sin_port = htons(port);
        inet_pton(AF_INET, IP, &(serverAddr.sin_addr.s_addr));

        int err = ::connect(socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
        if (err == SOCKET_ERROR)
            return E_CONNECT;

        return E_OK;
    }
    ERR shutdown(int how) {
        if (socket == INVALID_SOCKET) return E_OK;

        int err = ::shutdown(socket, how);
        if (err == SOCKET_ERROR)
            return W_SHUTDOWN;

        if (status) status--;

        return E_OK;
    }
    ERR close() {
        if (socket == INVALID_SOCKET) return E_OK;

        int err = ::closesocket(socket);
        if (err == SOCKET_ERROR)
            return W_CLOSE_SOCKET;

        socket = INVALID_SOCKET;
        return E_OK;
    }

    std::atomic_uint8_t status; // Count of opened streams (read/write) of the socket
private:
    SOCKET socket;
};