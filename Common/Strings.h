#pragma once

#include <algorithm>
#include <string>

template <typename T, T max_len>
struct PlanarString {
    PlanarString (std::string_view str) {
      length = std::min(str.length(), max_len);
      memcpy(&string, str.data(), length);
    };
    T length;
    char string[max_len];
};