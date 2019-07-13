#pragma once
#include <queue>
#include <mutex>

constexpr uint16_t NET_BUFFER_SIZE = 8192;

extern std::mutex msg_mutex;

enum class ConsoleColor : uint16_t {
	// Тёмные цвета
	Success = 2,
	Info    = 3,
	Danger  = 4,
	Warning = 6,
	Default = 7,
	// Насыщенные цвета
	SuccessHighlighted = 10,
	InfoHighlighted    = 11,
	DangerHighlighted  = 12,
	WarningHighlighted = 14,
	DefaultHighlighted = 15,
};

void log_raw_nonl(std::string_view str); // without new line
void log_raw_colored(ConsoleColor color, std::string_view str);
void log_raw(std::string_view str);

void log_nonl(const char* fmt, ...); // without new line
void log_colored(ConsoleColor color, const char* fmt, ...);
void log(const char* fmt, ...);

struct Packet {
	Packet(uint32_t ID, const char* data, size_t size, bool needACK);
	~Packet();
	uint32_t ID;
	size_t size;
	bool needACK;
	char* data;
};

typedef std::shared_ptr<Packet> PacketPtr;

//std::ostream& operator<< (std::ostream& os, const Packet& val);

class PacketFactory {
public:
	PacketFactory();
	PacketPtr create(const char* data, size_t size, bool needACK);
private:
	uint32_t ID;
};

extern PacketFactory packetFactory;


// Print WSA errors
const char* __get_filename(const char* file);

#define __FILENAME__ __get_filename(__FILE__)

void __wsa_print_err(const char* file, int line);

#define wsa_print_err() __wsa_print_err(__FILENAME__, __LINE__)

// Set description to current thread
void setThreadDesc(const wchar_t* desc);

// Get description of current thread
void getThreadDesc(wchar_t** dest);