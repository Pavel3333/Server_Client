#pragma once

#include "pch.h"

#include <mutex>
#include <queue>
#include <thread>
#include <functional>

constexpr uint16_t NET_BUFFER_SIZE = 8192;

enum class ERROR_TYPE : uint8_t {
	OK = 0,        // Без ошибок
	WARNING,       // Предупреждение
	SOFT_ERROR,    // Ошибка
	CRITICAL_ERROR // Критическая ошибка
};

enum class ClientState : uint8_t {
	InitWinSock,
	CreateReadSocket,
	CreateWriteSocket,
	Send,
	Shutdown,
	Receive,
	CloseSockets
};

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

void log_raw_nonl(const char* str); // without new line
void log_raw_colored(ConsoleColor color, const char* str);
void log_raw(const char* str);
void log_raw(std::string_view str);

void log_nonl(const char* fmt, ...); // without new line
void log_colored(ConsoleColor color, const char* fmt, ...);
void log(const char* fmt, ...);

struct Packet {
	Packet(uint32_t ID, const char* data, size_t size, bool needACK);
	~Packet();
	uint32_t ID;
	char* data;
	size_t size;
	bool needACK;
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
void __wsa_print_err(const char* file, int line);

#define wsa_print_err() __wsa_print_err(__FILE__, __LINE__)

// Set description to current thread
void setThreadDesc(const wchar_t* desc);

// Get description of current thread
void getThreadDesc(wchar_t** dest);

class Client {
public:
	Client(PCSTR IP, uint16_t readPort, uint16_t writePort);
	~Client();

	std::vector<PacketPtr> receivedPackets;
	std::vector<PacketPtr> sendedPackets;

	std::queue<PacketPtr> mainPackets;
	std::vector<PacketPtr> syncPackets;

	bool isRunning();

	int init();
	int disconnect();
private:
	SOCKET connect2server(uint16_t port);

	int handshake();

	void createThreads();

	int ack_handler(PacketPtr packet);
	int any_packet_handler(PacketPtr packet);

	int handlePacketIn(std::function<int(PacketPtr)>handler);
	int handlePacketOut(PacketPtr packet);

	void receiverThread();
	void senderThread();

	int sendData(PacketPtr packet);
	int receiveData(PacketPtr dest);

	void setState(ClientState state);

	std::thread receiver;
	std::thread sender;

	ClientState state;

	PCSTR IP;

	uint16_t readPort;
	uint16_t writePort;

	bool started;

	SOCKET readSocket;
	SOCKET writeSocket;
};