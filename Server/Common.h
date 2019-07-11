#pragma once

constexpr uint16_t NET_BUFFER_SIZE = 8192;

struct Packet {
	Packet(const char* data = nullptr, size_t size = 0, bool needACK = false);
	~Packet();
	char* data;
	size_t size;
	bool needACK;
};

typedef std::shared_ptr<Packet> PacketPtr;