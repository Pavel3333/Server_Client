#pragma once
#include <iostream>
#include <string_view>
#include <thread>
#include <mutex>

// Client actual version
const uint32_t CLIENT_VER = 1;

// Login string max size
const uint8_t  LOGIN_MAX_SIZE = 24;

// Network buffer size
const uint16_t NET_BUFFER_SIZE = 8192;

// Timeout, in seconds
const int TIMEOUT = 3;

#include "Log.h"
#include "Packet.h"
#include "Error.h"
