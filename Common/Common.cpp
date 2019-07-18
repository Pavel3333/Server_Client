#include <string_view>
#include <iostream>
#include "Common.h"

using std::cout;
using std::endl;

std::mutex msg_mutex;

static void printThreadDesc() {
	wchar_t* desc;
	getThreadDesc(&desc);

	wprintf(L"%-22s", desc);
}

static void setConsoleColor(ConsoleColor color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));
}


void log_raw_nonl(std::string_view str) {
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	cout << str;
}

void log_raw_colored(ConsoleColor color, std::string_view str) {
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	setConsoleColor(color);
	cout << str << endl;
	setConsoleColor(ConsoleColor::Default);
}

void log_raw(std::string_view str) {
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	cout << str << endl;
}


void log_nonl(const char* fmt, ...) {
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
}

void log_colored(ConsoleColor color, const char* fmt, ...) {
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	setConsoleColor(color);
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	setConsoleColor(ConsoleColor::Default);
	cout << endl;
}

void log(const char* fmt, ...) {
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	cout << endl;
}


Packet::Packet(uint32_t ID, std::string_view data, bool needACK)
	: std::vector<char>()
{
	this->resize(sizeof(packet_header) + data.size());

	header = reinterpret_cast<packet_header*>(this->data());
	header->fullsize = (uint32_t)(sizeof(packet_header) + data.size());
	header->ID = ID;
	header->needACK = needACK;

	memcpy(this->data() + sizeof(packet_header), data.data(), data.size());
}


Packet::Packet(std::string_view data)
	: std::vector<char>(data.size())
{
	memcpy(this->data(), data.data(), data.size());
	this->header = reinterpret_cast<packet_header*>(this->data());
}

std::ostream& operator<< (std::ostream& os, Packet& packet)
{
	os << "{" << endl
		<< "  ID     : " << packet.getID()     << endl
		<< "  size   : " << packet.size()      << endl
		<< "  needACK: " << packet.isNeedACK() << endl
		<< "  data   : \"";

	os.write(packet.data(), packet.size());

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
	std::lock_guard lock(msg_mutex);
	int err = WSAGetLastError();

	char err_msg[256] = "";
	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		err_msg, sizeof(err_msg), NULL);

	setConsoleColor(ConsoleColor::DangerHighlighted);
	printf("%s:%d - WSA Error %d:\n%s", file, line, err, err_msg);
	setConsoleColor(ConsoleColor::Default);
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
void getThreadDesc(wchar_t** dest)
{
	GetThreadDescription(GetCurrentThread(), dest);
}
