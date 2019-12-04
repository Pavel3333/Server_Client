#pragma once

template <class T>
struct PlanarString {
    T size;
    char string[0]; // TODO: dynamic size
};