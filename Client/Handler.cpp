#include "pch.h"
#include "Handler.h"

// Обработать пакет ACK
ERR Handler::ack_handler(PacketPtr packet)
{
    LOG::raw_colored(CC_InfoHL, std::string_view(packet->getData(), packet->getDataSize()));
    return E_OK;
}

// Обработать любой входящий пакет
ERR Handler::handle_packet(PacketPtr packet)
{
    switch (packet->getDataType()) {
    case DT_MSG:
    case DT_AUTH_SERVER:
        LOG::raw_colored(CC_InfoHL, std::string_view(packet->getData(), packet->getDataSize()));
    case DT_EMPTY:
        break;
    default:
        return E_UNKNOWN_DATATYPE;
    }

    // if (packet->isNeedACK())
    // TODO: send ACK to the server

    return E_OK;
}