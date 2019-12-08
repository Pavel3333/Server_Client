#pragma once

#include "Common.h"

class Handler {
public:
    static ERR handle_packet(PacketPtr packet);
private:
    static ERR handle_ack(PacketPtr packet);
};