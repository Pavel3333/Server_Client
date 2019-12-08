#pragma once
#include "Common.h"


class Handler {
private:
    static ERR handle_ack(PacketPtr packet);

public:
    static ERR handle_packet(PacketPtr packet);
};
