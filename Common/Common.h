#pragma once
#include <iostream>
#include <string_view>
#include <thread>
#include <mutex>

// Client actual version
constexpr uint32_t CLIENT_VER = 1;

// Structure bit field size
constexpr uint8_t LOGIN_BITCNT = 5;
constexpr uint8_t PWD_BITCNT   = 5;
constexpr uint8_t TOKEN_BITCNT = 5;

// Max string size
constexpr uint8_t LOGIN_MAX_SIZE = (1 << LOGIN_BITCNT);
constexpr uint8_t PWD_MAX_SIZE   = (1 << PWD_BITCNT);
constexpr uint8_t TOKEN_MAX_SIZE = (1 << TOKEN_BITCNT);

// Network buffer size
constexpr uint16_t NET_BUFFER_SIZE = 8192;

// Timeout, in seconds
constexpr int TIMEOUT = 3;

#include "Log.h"
#include "Packet.h"
#include "Error.h"
