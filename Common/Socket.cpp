#include "Socket.h"

ClientError Socket::setTimeout(uint32_t timeout, int option)
{
    if (socket == INVALID_SOCKET)
        return CE_UNKNOWN;

    uint32_t value = timeout * 1000;

    int err = setsockopt(socket, SOL_SOCKET, option, (char*)&value, sizeof(value));
    if (err == SOCKET_ERROR)
        return CW_SET_TIMEOUT;

    return CE_OK;
}

ClientError Socket::init(PCSTR ip_str, uint16_t port, IPPROTO protocol)
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
        return CE_UNKNOWN_PROTOCOL;
    }

    socket = ::socket(AF_INET, sock_type, protocol);
    if (socket == INVALID_SOCKET)
        return CE_CREATE_SOCKET;

    status = 2;

    return CE_OK;
}

ClientError Socket::connect(PCSTR ip_str, uint16_t port)
{
    if (socket == INVALID_SOCKET)
        return CE_UNKNOWN;

    sockaddr_in serverAddr{ AF_INET, htons(port) };

    // socketDesc.sin_family = AF_INET;
    // socketDesc.sin_port = htons(port);
    inet_pton(AF_INET, ip_str, &(serverAddr.sin_addr.s_addr));

    int err = ::connect(socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (err == SOCKET_ERROR) {
        switch (WSAGetLastError()) {
        case WSAETIMEDOUT:
            // Таймаут
            return CW_TIMEOUT;
        default:
            // Критическая ошибка
            return CE_CONNECT;
        }
    }

    return CE_OK;
}

ClientError Socket::shutdown(int how)
{
    if (socket == INVALID_SOCKET)
        return CE_OK;

    int err = ::shutdown(socket, how);
    if (err == SOCKET_ERROR)
        return CW_SHUTDOWN;

    if (status)
        status--;

    return CE_OK;
}

ClientError Socket::close()
{
    if (socket == INVALID_SOCKET)
        return CE_OK;

    int err = ::closesocket(socket);
    if (err == SOCKET_ERROR)
        return CW_CLOSE_SOCKET;

    socket = INVALID_SOCKET;
    return CE_OK;
}
