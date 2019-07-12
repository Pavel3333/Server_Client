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

// Print WSA errors
void __wsa_print_err(const char* file, uint16_t line);

#define wsa_print_err() __wsa_print_err(__FILE__, __LINE__)