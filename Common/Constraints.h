#pragma once

// Client actual version
const uint32_t CLIENT_VER = 1;

// Structure bit field size
const uint8_t LOGIN_BITCNT = 5;
const uint8_t PWD_BITCNT = 5;
const uint8_t TOKEN_BITCNT = 5;

// Max string size
const uint8_t LOGIN_MAX_SIZE = (1 << LOGIN_BITCNT);
const uint8_t PWD_MAX_SIZE = (1 << PWD_BITCNT);
const uint8_t TOKEN_MAX_SIZE = (1 << TOKEN_BITCNT);

// Network buffer size
const uint16_t NET_BUFFER_SIZE = 8192;

// Timeout, in seconds
const int TIMEOUT = 3;
