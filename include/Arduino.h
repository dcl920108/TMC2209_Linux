#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

typedef uint8_t byte;
typedef bool boolean;

inline void delay(unsigned long ms) {}
inline void delayMicroseconds(unsigned int us) {}
inline unsigned long millis() { return 0; }
