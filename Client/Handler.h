#pragma once

#include "Common.h"

class Handler {
public:
    static ERR ack_handler(PacketPtr packet);
    static ERR handle_packet(PacketPtr packet);
};