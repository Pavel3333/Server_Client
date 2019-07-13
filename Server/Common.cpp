#include "pch.h"
#include <string_view>
#include "Common.h"

std::mutex msg_mutex;
static std::mutex pf_mutex;

static void printThreadDesc() {
	std::wstring threadDesc;
	threadDesc.reserve(32);

	wchar_t* desc;
	getThreadDesc(&desc);

	wsprintfW(threadDesc.data(), L"[%s]", desc);
	wprintf(L"%-11s", threadDesc.data());
}

static void setConsoleColor(ConsoleColor color) { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (uint16_t)color); }


void log_raw_nonl(std::string_view str) {
	msg_mutex.lock();
	printThreadDesc();
	cout << str;
	msg_mutex.unlock();
}

void log_raw_colored(ConsoleColor color, std::string_view str) {
	msg_mutex.lock();
	printThreadDesc();
	setConsoleColor(color);
	cout << str << endl;
	setConsoleColor(ConsoleColor::Default);
	msg_mutex.unlock();
}

void log_raw(std::string_view str) {
	msg_mutex.lock();
	printThreadDesc();
	cout << str << endl;
	msg_mutex.unlock();
}


void log_nonl(const char* fmt, ...) {
	msg_mutex.lock();
	printThreadDesc();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	msg_mutex.unlock();
}

void log_colored(ConsoleColor color, const char* fmt, ...) {
	msg_mutex.lock();
	printThreadDesc();
	setConsoleColor(color);
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	setConsoleColor(ConsoleColor::Default);
	cout << endl;
	msg_mutex.unlock();
}

void log(const char* fmt, ...) {
	msg_mutex.lock();
	printThreadDesc();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	cout << endl;
	msg_mutex.unlock();
}


Packet::Packet(uint32_t ID, const char* data, size_t size, bool needACK)
	: ID(ID)
	, size(size)
	, needACK(needACK)
{
	this->data = new char[size];
	memcpy(this->data, data, size);
}

Packet::~Packet() { delete[] this->data; }


//std::ostream& operator<< (std::ostream& os, const Packet& packet)
//{
//	os << "Packet: " << packet.size << ", " << "data: " << std::string_view(packet.data, packet.size);
//	return os;
//}

PacketFactory::PacketFactory()
	: ID(0)
{}

PacketPtr PacketFactory::create(const char* data, size_t size, bool needACK)
{
	pf_mutex.lock();
	PacketPtr result = std::make_shared<Packet>(this->ID++, data, size, needACK);
	pf_mutex.unlock();
	return result;
}

PacketFactory packetFactory;


void __wsa_print_err(const char* file, int line)
{
	msg_mutex.lock();
	int err = WSAGetLastError();

	char err_msg[256] = "";
	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		err_msg, sizeof(err_msg), NULL);

	printf("%s:%d - WSA Error %d:\n%s", file, line, err, err_msg);
	msg_mutex.unlock();
}

// Set description to current thread
void setThreadDesc(const wchar_t* desc) { SetThreadDescription(GetCurrentThread(), desc); }

// Get description of current thread
void getThreadDesc(wchar_t** dest) { GetThreadDescription(GetCurrentThread(), dest); }