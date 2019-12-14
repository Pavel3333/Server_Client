#pragma once

#include "Common.h"

class Handler {
public:
    static ClientError handle_packet(PacketPtr packet);
private:
    static ClientError handle_ack(PacketPtr packet);
};