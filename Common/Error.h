#pragma once

enum class ERROR_TYPE {
    OK = 0, // Без ошибок
    WARNING, // Предупреждение
    SOFT_ERROR, // Ошибка
    CRITICAL_ERROR // Критическая ошибка
};

enum ERR {
    W_HAMDLE_IN_PACKET_EMPTY,

    W_TIMEOUT = -5,
    W_SET_TIMEOUT,
    W_SHUTDOWN,
    W_CLOSE_SOCKET,

    W_MSG_SIZE,

    E_OK,

    E_START_SERVER,

    E_INIT_WINSOCK,

    E_CREATE_SOCKET,
    E_CONNECT,

    E_AUTH_CLIENT_SEND,

    E_AUTH_SERVER_RECV,
    E_AUTH_SERVER_EMPTY,
    E_AUTH_SERVER_SIZE,
    E_AUTH_SERVER_GOTERR,

    E_HANDLE_OUT_SEND,

    E_SEND,
    E_RECV,

    E_CONN_CLOSED,

    E_UNKNOWN_PROTOCOL,
    E_UNKNOWN_DATATYPE,

    E_UNKNOWN
};
static_assert(E_OK == 0);

enum SERVER_ERR {
    SE_OK = 0,

    SE_UNKNOWN
};
static_assert(SE_OK == 0);

#define SUCCESS(err) (err == 0)
#define WARNING(err) (err < 0)
#define _ERROR(err) (err > 0)