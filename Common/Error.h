#pragma once

enum class ERROR_TYPE {
    OK = 0,        // Без ошибок
    WARNING,       // Предупреждение
    SOFT_ERROR,    // Ошибка
    CRITICAL_ERROR // Критическая ошибка
};

enum ERR {
    W_TIMEOUT = -4,

    W_SET_TIMEOUT,
    W_SHUTDOWN,
    W_CLOSE_SOCKET,

    E_OK,

    E_START_SERVER,

    E_UNKNOWN_PROTOCOL,
    E_CREATE_SOCKET,
    E_CONNECT,

    E_UNKNOWN
};
static_assert(E_OK == 0);

enum SERVER_ERR {
    SE_OK = 0,

    

    SE_UNKNOWN
};
static_assert(SE_OK == 0);

#define SUCCESS(err) (err == 0)
#define WARNING(err) (err <  0)
#define _ERROR(err)  (err >  0)