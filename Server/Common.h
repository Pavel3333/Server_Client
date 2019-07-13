#pragma once
#include <queue>
#include <mutex>

constexpr uint16_t NET_BUFFER_SIZE = 8192;

extern std::mutex msg_mutex;

void log_raw_nonl(const char* str); // without new line
void log_raw(const char* str);

void log_nonl(const char* fmt, ...); // without new line
void log(const char* fmt, ...);

struct Packet {
	Packet(const char* data, size_t size, bool needACK);
	~Packet();
	char* data;
	size_t size;
	bool needACK;
};

typedef std::shared_ptr<Packet> PacketPtr;

//std::ostream& operator<< (std::ostream& os, const Packet& val);

// Print WSA errors
void __wsa_print_err(const char* file, int line);

#define wsa_print_err() __wsa_print_err(__FILE__, __LINE__)

// Set description to current thread
void setThreadDesc(const wchar_t* desc);

// Get description of current thread
void getThreadDesc(wchar_t** dest);