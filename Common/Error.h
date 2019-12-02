#pragma once

enum class ERROR_TYPE {
    OK = 0,        // Без ошибок
    WARNING,       // Предупреждение
    SOFT_ERROR,    // Ошибка
    CRITICAL_ERROR // Критическая ошибка
};

enum class ERR {
    OK = 0,

    E_START_SERVER,

    UNKNOWN
};

#define SUCCESS(err) (err == ERR::OK)
#define WARNING(err) (err <  ERR::OK)
#define _ERROR(err)   (err >  ERR::OK)