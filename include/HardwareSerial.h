#pragma once
#include "Stream.h"
#include <cstdint>
#include <cstddef>

class HardwareSerial : public Stream {
public:
    virtual void begin(long baud) {}
    virtual void end() {}
    virtual int available() { return 0; }
    virtual int read() { return -1; }
    virtual size_t write(uint8_t) { return 0; }
    virtual size_t write(const uint8_t* buf, size_t len) { return 0; }
    virtual void flush() {}
};
