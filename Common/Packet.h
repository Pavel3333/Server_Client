#pragma once
#include <algorithm>
#include <atomic>

#include "Error.h"
#include "Socket.h"

enum DataType {
    DT_EMPTY,
    DT_ACK,

    DT_AUTH_CLIENT,
    DT_AUTH_SERVER,

    DT_MSG,

    DT_UNKNOWN
};

class Packet;
typedef std::shared_ptr<Packet> PacketPtr;

class Packet : public std::vector<char> {
public:
    Packet(uint32_t ID, DataType type, bool needACK, size_t data_size, const char* data = nullptr);
    Packet(std::string_view data);

    uint32_t getID() const
    {
        return header->ID;
    }

    bool isNeedACK() const
    {
        return header->needACK;
    }

    const char* getData() const
    {
        return this->data() + sizeof(PacketHeader) + sizeof(DataHeader);
    }

    uint32_t getDataSize() const
    {
        return data_header->data_size;
    }

    DataType getDataType() const
    {
        return data_header->type;
    }

    std::string_view readData(size_t size)
    {
        using std::string_view;

        size = availableSize(size);
        if (!size)
            return nullptr;

        string_view data = string_view(getData() + data_offset, size);
        data_offset += size;
        return data;
    }

    void writeData(std::string_view data)
    {
        size_t size = availableSize(data.size());

        memcpy((char*)getData() + data_offset, data.data(), size);
        data_offset += size;
    }

    int send(Socket& sock)
    {
        return ::send(sock.getSocket(), data(), static_cast<int>(size()), 0);
    }

private:
    uint32_t data_offset;

#ifdef min
#undef min
#endif
    size_t availableSize(size_t size)
    {
        if (data_offset >= data_header->data_size)
            return 0;
        size_t available = std::min(data_header->data_size - data_offset, size);
        return size;
    }

#pragma pack(push, 1)
    struct PacketHeader {
        uint32_t fullsize;
        uint32_t ID;
        bool needACK;
    } * header;

    struct DataHeader {
        DataType type;
        uint32_t data_size;
    } * data_header;

    // check packing
    static_assert(sizeof(PacketHeader) == 9);
    static_assert(sizeof(DataHeader) == 8);
#pragma pack(pop)
};

// TODO: struct DataPacket - пакет, у которого в заголовке token (для идентификации отправителя)

std::ostream& operator<<(std::ostream& os, Packet& packet);

// Packet Factory Singleton
class PacketFactory {
public:
    static PacketPtr create(DataType type, const char* data, size_t size, bool needACK)
    {
        using std::make_shared;
        using std::string_view;

        return make_shared<Packet>(getID(), type, needACK, size, data);
    }

    static PacketPtr create(const char* data, size_t size)
    {
        using std::make_shared;
        using std::string_view;

        return make_shared<Packet>(string_view(data, size));
    }

    template <class T>
    static PacketPtr create_from_struct(DataType type, T& packet, bool needACK)
    {
        return create(
            type,
            reinterpret_cast<const char*>(&packet),
            sizeof(packet),
            needACK);
    }

private:
    static std::atomic_uint getID()
    {
        static std::atomic_uint ID;
        return ID++;
    }

    // copy guard
    PacketFactory() {}
    PacketFactory(const PacketFactory&) {}
    PacketFactory& operator=(PacketFactory&) {}
};

// Auth packets

#pragma pack(push, 1)
struct ClientAuthHeader {
    uint8_t loginSize : LOGIN_BITCNT;
    uint8_t passSize  : PWD_BITCNT;
};

struct ServerAuthHeader {
    SERVER_ERR errorCode;
    uint8_t tokenSize : TOKEN_BITCNT;
};

// check packing
static_assert(sizeof(ServerAuthHeader) == 5);
static_assert(sizeof(ClientAuthHeader) == 2);
#pragma pack(pop)

// ACK packet

#pragma pack(push, 1)
struct ClientACK {
    ERR errorCode;
    uint8_t tokenSize : TOKEN_BITCNT;
    uint32_t acknowledgedID;
};

struct ServerACK {
    SERVER_ERR errorCode;
    uint32_t acknowledgedID;
};

// check packing
static_assert(sizeof(ClientACK) == 9);
static_assert(sizeof(ServerACK) == 8);
#pragma pack(pop)