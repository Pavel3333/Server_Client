#include <string_view>
#include "Common.h"
#include <Windows.h>
#include <iostream>

using std::cout;
using std::endl;

std::mutex msg_mutex;

static void printThreadDesc() {
	wchar_t* desc;
	getThreadDesc(&desc);

	wprintf(L"%-22s", desc);
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


std::ostream& operator<< (std::ostream& os, Packet& packet)
{
	os << "{" << endl
		<< "  ID     : " << packet.ID << endl
		<< "  size   : " << packet.size << endl
		<< "  needACK: " << packet.needACK << endl
		<< "  data   : \"";

	os.write(packet.data, packet.size);

	os << "\"" << endl
		<< "}" << endl;

	return os;
}


const char* __get_filename(const char* file) {
	const char* last_slash = strrchr(file, '\\');
	return last_slash ? last_slash + 1 : file;
}

void __wsa_print_err(const char* file, int line)
{
	msg_mutex.lock();
	int err = WSAGetLastError();

	char err_msg[256] = "";
	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		err_msg, sizeof(err_msg), NULL);

	setConsoleColor(ConsoleColor::DangerHighlighted);
	printf("%s:%d - WSA Error %d:\n%s", file, line, err, err_msg);
	setConsoleColor(ConsoleColor::Default);

	msg_mutex.unlock();
}

// Set description to current thread
void setThreadDesc(const wchar_t* desc) { SetThreadDescription(GetCurrentThread(), desc); }
// Set description to current thread with formatting ID
void setThreadDesc(const wchar_t* fmt, uint16_t ID) {
	std::wstring buf;
	buf.reserve(32);

	wsprintfW(buf.data(), fmt, ID);

	SetThreadDescription(GetCurrentThread(), buf.data());
}

// Get description of current thread
void getThreadDesc(wchar_t** dest) { GetThreadDescription(GetCurrentThread(), dest); }