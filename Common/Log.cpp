#include "Common.h"

using std::cout;
using std::endl;

std::mutex msg_mutex;


static void printThreadDesc()
{
	wchar_t* desc;
	getThreadDesc(&desc);

	wprintf(L"%-22s", desc);
}

static void setConsoleColor(ConsoleColor color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));
}

void LOG::raw_nonl(std::string_view str)
{
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	cout << str;
}

void LOG::raw_colored(ConsoleColor color, std::string_view str)
{
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	setConsoleColor(color);
	cout << str << endl;
	setConsoleColor(ConsoleColor::Default);
}

void LOG::raw(std::string_view str)
{
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	cout << str << endl;
}

void LOG::nonl(const char* fmt, ...)
{
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
}

void LOG::colored(ConsoleColor color, const char* fmt, ...)
{
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

void LOG::log(const char* fmt, ...)
{
	std::lock_guard lock(msg_mutex);
	printThreadDesc();
	va_list args;
	va_start(args, fmt);
	vprintf_s(fmt, args);
	va_end(args);
	cout << endl;
}

const char* __get_filename(const char* file)
{
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
void setThreadDesc(const wchar_t* desc)
{
	SetThreadDescription(GetCurrentThread(), desc);
}

// Set description to current thread with formatting ID
void setThreadDesc(const wchar_t* fmt, uint16_t ID)
{
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
