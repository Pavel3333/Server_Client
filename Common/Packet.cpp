#include "Common.h"


Packet::Packet(uint32_t ID, std::string_view data, bool needACK)
	: std::vector<char>()
{
	this->resize(sizeof(packet_header) + data.size());

	header = reinterpret_cast<packet_header*>(this->data());
	header->fullsize = (uint32_t)(sizeof(packet_header) + data.size());
	header->ID = ID;
	header->needACK = needACK;

	memcpy(this->data() + sizeof(packet_header), data.data(), data.size());
}


Packet::Packet(std::string_view data)
	: std::vector<char>(data.size())
{
	memcpy(this->data(), data.data(), data.size());
	this->header = reinterpret_cast<packet_header*>(this->data());
}


std::ostream& operator<< (std::ostream& os, Packet& packet)
{
	using std::cout;
	using std::endl;

	os << "{" << endl
		<< "  ID     : " << packet.getID()     << endl
		<< "  size   : " << packet.size()      << endl
		<< "  needACK: " << packet.isNeedACK() << endl
		<< "  data   : \"";

	os.write(packet.data(), packet.size());

	os << "\"" << endl
		<< "}" << endl;

	return os;
}

