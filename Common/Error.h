#pragma once

enum class ERROR_TYPE {
    OK = 0,        // ��� ������
    WARNING,       // ��������������
    SOFT_ERROR,    // ������
    CRITICAL_ERROR // ����������� ������
};

enum class ERR {
    OK = 0,

    E_START_SERVER,

    UNKNOWN
};

#define SUCCESS(err) (err == ERR::OK)
#define WARNING(err) (err <  ERR::OK)
#define _ERROR(err)   (err >  ERR::OK)