#pragma once

enum class ERROR_TYPE {
    OK = 0,        // Без ошибок
    WARNING,       // Предупреждение
    SOFT_ERROR,    // Ошибка
    CRITICAL_ERROR // Критическая ошибка
};

enum ClientError {
    CW_HANDLE_IN_PACKET_EMPTY = -7,

    CW_NEED_ACK,

    CW_TIMEOUT ,
    CW_SET_TIMEOUT,
    CW_SHUTDOWN,
    CW_CLOSE_SOCKET,

    CW_MSG_SIZE,

    CE_OK,

    CE_INIT_WINSOCK,

    CE_CREATE_SOCKET,
    CE_CONNECT,

    CE_AUTH_SEND,

    CE_AUTH_RECV,
    CE_AUTH_RECV_EMPTY,
    CE_AUTH_RECV_SIZE,
    CE_AUTH_RECV_GOTERR,

    CE_SERVER_ACK_SIZE,

    CE_ACKING_PACKET_NOT_FOUND,

    CE_HANDLE_OUT_SEND,

    CE_SEND,

    CE_RECV,
    CE_RECV_SIZE,

    CE_CONN_CLOSED,

    CE_UNKNOWN_PROTOCOL,
    CE_UNKNOWN_DATATYPE,

    CE_UNKNOWN
};
static_assert(CE_OK == 0);

enum ServerError {
    SW_ALREADY_STARTED = -1,

    SE_OK,

    SE_START_SERVER,

    SE_INIT_WINSOCK,

    SE_CREATE_SOCKET,

    SE_UNKNOWN
};
static_assert(SE_OK == 0);

#define SUCCESS(err) (err == 0)
#define WARNING(err) (err < 0)
#define _ERROR(err) (err > 0)