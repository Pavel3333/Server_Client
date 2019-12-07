#include "Socket.h"

ERR Socket::setTimeout(uint32_t timeout, int option)
{
    if (socket == INVALID_SOCKET)
        return E_UNKNOWN;

    uint32_t value = timeout * 1000;

    int err = setsockopt(socket, SOL_SOCKET, option, (char*)&value, sizeof(value));
    if (err == SOCKET_ERROR)
        return W_SET_TIMEOUT;

    return E_OK;
}

ERR Socket::init(PCSTR ip_str, uint16_t port, IPPROTO protocol)
{
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

ERR Socket::connect(PCSTR ip_str, uint16_t port)
{
    if (socket == INVALID_SOCKET)
        return E_UNKNOWN;

    sockaddr_in serverAddr{ AF_INET, htons(port) };

    // socketDesc.sin_family = AF_INET;
    // socketDesc.sin_port = htons(port);
    inet_pton(AF_INET, ip_str, &(serverAddr.sin_addr.s_addr));

    int err = ::connect(socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (err == SOCKET_ERROR) {
        switch (WSAGetLastError()) {
        case WSAETIMEDOUT:
            // Таймаут
            return W_TIMEOUT;
        default:
            // Критическая ошибка
            return E_CONNECT;
        }
    }

    return E_OK;
}

ERR Socket::shutdown(int how)
{
    if (socket == INVALID_SOCKET)
        return E_OK;

    int err = ::shutdown(socket, how);
    if (err == SOCKET_ERROR)
        return W_SHUTDOWN;

    if (status)
        status--;

    return E_OK;
}

ERR Socket::close()
{
    if (socket == INVALID_SOCKET)
        return E_OK;

    int err = ::closesocket(socket);
    if (err == SOCKET_ERROR)
        return W_CLOSE_SOCKET;

    socket = INVALID_SOCKET;
    return E_OK;
}
