#pragma once
#include <string_view>
#include <thread>
#include <mutex>
#include <atomic>
#include <winsock2.h>


constexpr uint16_t NET_BUFFER_SIZE = 8192;
constexpr int      TIMEOUT = 3;    // Таймаут в секундах

extern std::mutex msg_mutex;

enum class ConsoleColor {
	// Тёмные цвета
	Success = 2,
	Info = 3,
	Danger = 4,
	Warning = 6,
	Default = 7,
	// Насыщенные цвета
	SuccessHighlighted = 10,
	InfoHighlighted = 11,
	DangerHighlighted = 12,
	WarningHighlighted = 14,
	DefaultHighlighted = 15,
};

void log_raw_nonl(std::string_view str); // without new line
void log_raw_colored(ConsoleColor color, std::string_view str);
void log_raw(std::string_view str);

void log_nonl(const char* fmt, ...); // without new line
void log_colored(ConsoleColor color, const char* fmt, ...);
void log(const char* fmt, ...);


class Packet: public std::vector<char> {
public:
	Packet(uint32_t ID, const char* data, size_t size, bool needACK);
	~Packet() = default;

	uint32_t ID;
	bool needACK;

	int send(SOCKET s) {
		return ::send(s, data(), static_cast<int>(size()), 0);
	}
};

typedef std::shared_ptr<Packet> PacketPtr;

class PacketFactory {
public:
	static PacketPtr create(const char* data, size_t size, bool needACK) {
		using std::make_shared;
		return make_shared<Packet>(getID(), data, size, needACK);
	}
private:
	static std::atomic_uint getID() {
		static std::atomic_uint ID;
		return ID++;
	}

	// Защита от копирования
	PacketFactory() {}
	PacketFactory(const PacketFactory&) {}
	PacketFactory& operator=(PacketFactory&) {}
};

// Print WSA errors
const char* __get_filename(const char* file);

#define __FILENAME__ __get_filename(__FILE__)

void __wsa_print_err(const char* file, int line);

#define wsa_print_err() __wsa_print_err(__FILENAME__, __LINE__)

// Set description to current thread
void setThreadDesc(const wchar_t* desc);

// Get description of current thread
void getThreadDesc(wchar_t** dest);
