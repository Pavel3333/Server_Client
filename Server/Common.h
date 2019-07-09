#pragma once

constexpr uint16_t NET_BUFFER_SIZE = 8192;

struct Packet {
	Packet(const char* data, size_t size = 0);
	~Packet();
	char* data;
	size_t size;
};