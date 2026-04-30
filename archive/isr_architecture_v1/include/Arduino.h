// #pragma once
// #include <cstdint>
// #include <cstddef>
// #include <cstring>

// typedef uint8_t byte;
// typedef bool boolean;

// inline void delay(unsigned long ms) {}
// inline void delayMicroseconds(unsigned int us) {}
// inline unsigned long millis() { return 0; }
// 4/3/2026
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>

typedef uint8_t byte;
typedef bool boolean;

#define HIGH 1
#define LOW  0
#define OUTPUT 1
#define INPUT  0

inline void delay(unsigned long ms) {}
inline void delayMicroseconds(unsigned int us) {}
inline unsigned long millis() { return 0; }
inline void pinMode(uint8_t pin, uint8_t mode) {}
inline void digitalWrite(uint8_t pin, uint8_t val) {}
inline int digitalRead(uint8_t pin) { return 0; }

inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline long constrain(long x, long a, long b) {
    return std::min(std::max(x, a), b);
}

#include "HardwareSerial.h"
