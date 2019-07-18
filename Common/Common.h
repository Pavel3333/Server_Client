#pragma once
#include <string_view>
#include <thread>
#include <mutex>
#include <atomic>
#include <winsock2.h>

constexpr uint8_t  LOGIN_MAX_SIZE  = 24;   // Login string max size
constexpr uint16_t NET_BUFFER_SIZE = 8192; // Network buffer size
constexpr int      TIMEOUT         = 3;    // Timeout, in seconds

extern std::mutex msg_mutex;

enum class ConsoleColor {
	// Dark colors
	Success = 2,
	Info = 3,
	Danger = 4,
	Warning = 6,
	Default = 7,
	// Rich colors
	SuccessHighlighted = 10,
	InfoHighlighted = 11,
	DangerHighlighted = 12,
	WarningHighlighted = 14,
	DefaultHighlighted = 15,
};

void log_raw_nonl(std::string_view str);                        //         without new line
void log_raw_colored(ConsoleColor color, std::string_view str); // colored with new line
void log_raw(std::string_view str);                             //         with new line

void log_nonl(const char* fmt, ...);                        //          formatted without new line
void log_colored(ConsoleColor color, const char* fmt, ...); // colored, formatted with new line
void log(const char* fmt, ...);                             //          formatted with new line


class Packet: public std::vector<char> {
public:
	Packet(uint32_t ID, std::string_view, bool needACK);
	Packet(std::string_view);

	uint32_t getID() const {
		return header->ID;
	}

	bool isNeedACK() const {
		return header->needACK;
	}

	const char* getData() const {
		return this->data() + sizeof(header);
	}

	uint32_t getDataSize() const {
		return header->fullsize - sizeof(header);
	}

	int send(SOCKET s) {
		return ::send(s, data(), static_cast<int>(size()), 0);
	}

private:
#pragma pack(push,1)
	struct packet_header {
		uint32_t fullsize;
		uint32_t ID;
		bool needACK;
	} *header;

	// check packing
	static_assert(sizeof(packet_header) == 9);
#pragma pack(pop)
};

std::ostream& operator<< (std::ostream& os, Packet& packet);

typedef std::shared_ptr<Packet> PacketPtr;

using std::make_shared;

// Синглтон фабрики пакетов
class PacketFactory {
public:
	static PacketPtr create(const char* data, size_t size, bool needACK) {
		return make_shared<Packet>(getID(), std::string_view(data, size), needACK);
	}
	static PacketPtr create(const char* data, size_t size) {
		return make_shared<Packet>(std::string_view(data, size));
	}
private:
	static std::atomic_uint getID() {
		static std::atomic_uint ID;
		return ID++;
	}

	// Защита от копирования
	PacketFactory()                          {}
	PacketFactory(const PacketFactory&)      {}
	PacketFactory& operator=(PacketFactory&) {}
};

// Hello-пакеты

#pragma pack(push,1)
struct ServerHelloPacket {
	uint32_t clientID;
};

struct ClientHelloPacket {
	ClientHelloPacket() {
		serverHelloPacketID = 0;
		memset(login, '\0', LOGIN_MAX_SIZE);
	}

	ClientHelloPacket(uint32_t ID, std::string_view login) {
		serverHelloPacketID = ID;
		loginSize           = (uint32_t)min(login.size(), LOGIN_MAX_SIZE - 1);
		memcpy(this->login, login.data(), loginSize);
		this->login[loginSize] = '\0';
	}

	uint32_t serverHelloPacketID;
	uint32_t loginSize;
	char     login[LOGIN_MAX_SIZE];
};

// check packing
static_assert(sizeof(ServerHelloPacket) == 4);
static_assert(sizeof(ClientHelloPacket) == 8 + LOGIN_MAX_SIZE);
#pragma pack(pop)


// Print WSA errors
const char* __get_filename(const char* file);

#define __FILENAME__ __get_filename(__FILE__)

void __wsa_print_err(const char* file, int line);

#define wsa_print_err() __wsa_print_err(__FILENAME__, __LINE__)

// Set description to current thread
void setThreadDesc(const wchar_t* desc);
void setThreadDesc(const wchar_t* fmt, uint16_t ID);

// Get description of current thread
void getThreadDesc(wchar_t** dest);
