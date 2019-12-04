#pragma once

#include <string.h>

#define min(a,b) (((a) > (b)) ? (a) : (b))

template <typename T, T max_len>
struct PlanarString {
    PlanarString (const char str[]) { // TODO: const char[] -> std::string_view
      length = min(strlen(str), max_len);
      memcpy(&string, str, length);
    };
    T length;
    char string[max_len];
};
