#pragma once
#include <atomic>
#include <winsock2.h>

struct Socket {
    Socket(SOCKET socket) { // TODO

    }
    SOCKET socket;
    std::atomic_uint8_t status; // Count of opened streams (read/write) of the socket
};