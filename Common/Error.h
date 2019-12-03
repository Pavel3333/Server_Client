#pragma once

enum class ERROR_TYPE {
    OK = 0,        // ��� ������
    WARNING,       // ��������������
    SOFT_ERROR,    // ������
    CRITICAL_ERROR // ����������� ������
};

enum ERR {
    E_OK = 0,

    E_START_SERVER,

    E_UNKNOWN
};

enum SERVER_ERR {
    SE_OK = 0,

    

    SE_UNKNOWN
};

#define SUCCESS(err) (err == 0)
#define WARNING(err) (err <  0)
#define _ERROR(err)  (err >  0)