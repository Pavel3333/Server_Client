#include "pch.h"
#include "Handler.h"
#include "Client.h"

#include <string_view>

// Обработать любой входящий пакет
ClientError Handler::handle_packet(PacketPtr packet)
{
    ClientError err = CE_OK;

    switch (packet->getDataType()) {
    case DT_ACK:
        err = handle_ack(packet);
        break;
    case DT_MSG:
    case DT_AUTH_SERVER:
        LOG::raw_colored(CC_InfoHL, std::string_view(packet->getData(), packet->getDataSize()));
    case DT_EMPTY:
        err = CE_OK;
        break;
    default:
        err = CE_UNKNOWN_DATATYPE;
    }

    if (packet->isNeedACK()) {
        std::string_view token = Client::getInstance().getToken();

        ClientACK ack{ err, static_cast<uint8_t>(token.size()), packet->getID() };
        PacketPtr ack_packet = PacketFactory::create_from_struct(DT_ACK, ack, false);
        ack_packet->writeData(token);

        Client::getInstance().sendPacket(ack_packet);
    }

    return err;
}

// Обработать пакет ACK
ClientError Handler::handle_ack(PacketPtr packet)
{
    if (packet->getDataType() != DT_ACK)
        return CE_UNKNOWN;
    else if (packet->getDataSize() < sizeof(ServerACK))
        // Packet size is incorrect
        return CE_SERVER_ACK_SIZE;
    
    auto serverACKHeader = reinterpret_cast<const ServerACK*>(
        packet->getData());

    uint32_t ackedID = serverACKHeader->acknowledgedID;

    auto begin = Client::getInstance().syncPackets.cbegin();
    auto end   = Client::getInstance().syncPackets.cend();

    auto acked_packet = std::find_if(
        begin,
        end,
        [ackedID](PacketPtr syncPacket) -> bool
        { return syncPacket->getID() == ackedID; }
    );

    if (acked_packet == end)
        return CE_ACKING_PACKET_NOT_FOUND;

    Client::getInstance().syncPackets.erase(acked_packet);

    return CE_OK;
}