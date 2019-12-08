#include "Common.h"


Packet::Packet(uint32_t ID, DataType type, bool needACK, size_t data_size, const char* data)
    : std::vector<char>()
    , data_offset(0)
{
    using std::string_view;

    uint32_t fullsize = sizeof(PacketHeader) + sizeof(DataHeader) + data_size;
    this->resize(fullsize);

    header = reinterpret_cast<PacketHeader*>(this->data());
    header->fullsize = fullsize;
    header->ID = ID;
    header->needACK = needACK;

    data_header = reinterpret_cast<DataHeader*>(this->data() + sizeof(PacketHeader));
    data_header->data_size = data_size;
    data_header->type = type;

    if (data)
        writeData(string_view(data, data_size));
}

Packet::Packet(std::string_view data)
    : std::vector<char>(data.size())
    , data_offset(0)
{
    memcpy(this->data(), data.data(), data.size());
    this->header = reinterpret_cast<PacketHeader*>(this->data());
    this->data_header = reinterpret_cast<DataHeader*>(this->data() + sizeof(PacketHeader));
}

std::ostream& operator<<(std::ostream& os, Packet& packet)
{
    using std::cout;
    using std::endl;

    os << "{" << endl
       << "  ID       : " << packet.getID() << endl
       << "  type     : " << packet.getDataType() << endl
       << "  needACK  : " << packet.isNeedACK() << endl
       << "  full size: " << packet.size() << endl
       << "  data size: " << packet.getDataSize() << endl
       << "  data     : \"";

    os.write(packet.getData(), packet.getDataSize());

    os << "\"" << endl
       << "}" << endl;

    return os;
}
