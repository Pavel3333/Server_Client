#include "pch.h"
#include <string_view>
#include "Common.h"


Packet::Packet(const char* data, size_t size, bool needACK)
	: needACK(needACK)
	, size(size)
{
	this->data = new char[size];
	memcpy(this->data, data, size);

#ifdef _DEBUG
	cout << *this << endl;
#endif
}


Packet::~Packet()
{
	delete[] this->data;
}


std::ostream& operator<< (std::ostream& os, const Packet& packet)
{
	os << "Packet: " << packet.size << ", " << "data: " << std::string_view(packet.data, packet.size);
	return os;
}
