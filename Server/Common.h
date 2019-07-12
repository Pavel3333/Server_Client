#pragma once

constexpr uint16_t NET_BUFFER_SIZE = 8192;

struct Packet {
	Packet(const char* data, size_t size, bool needACK);
	~Packet();
	char* data;
	size_t size;
	bool needACK;
};

typedef std::shared_ptr<Packet> PacketPtr;

std::ostream& operator<< (std::ostream& os, const Packet& val);
