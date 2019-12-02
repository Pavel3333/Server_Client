#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>

extern std::mutex msg_mutex;


enum class ConsoleColor {
	// Dark colors
	Success = FOREGROUND_GREEN,
	Info    = FOREGROUND_GREEN | FOREGROUND_BLUE,
	Danger  = FOREGROUND_RED,
	Warning = FOREGROUND_RED | FOREGROUND_GREEN,
	Default = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,

	// Rich colors
	SuccessHighlighted = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
	InfoHighlighted    = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
	DangerHighlighted  = FOREGROUND_INTENSITY | FOREGROUND_RED,
	WarningHighlighted = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
	DefaultHighlighted = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
};

// Logger
class LOG {
public:
    // without new line
    static void raw_nonl(std::string_view str);

    // colored with new line
    static void raw_colored(ConsoleColor color, std::string_view str);

    // with new line
    static void raw(std::string_view str);

    // formatted without new line
    static void nonl(const char* fmt, ...);

    // colored, formatted with new line
    static void colored(ConsoleColor color, const char* fmt, ...);

    // formatted with new line
    static void log(const char* fmt, ...);
};



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
