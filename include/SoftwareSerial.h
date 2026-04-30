#pragma once
#include "Stream.h"

class SoftwareSerial : public Stream {
public:
    SoftwareSerial(int rx, int tx) {}
    void begin(long baud) {}
    void end() {}
    int available() { return 0; }
    int read() { return -1; }
    size_t write(uint8_t) { return 0; }
};
