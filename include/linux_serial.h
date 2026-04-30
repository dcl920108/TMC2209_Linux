#pragma once
#include "HardwareSerial.h"
#include <cstdint>
#include <cstddef>

class LinuxSerial : public HardwareSerial {
public:
    explicit LinuxSerial(const char* port = "/dev/ttyAMA2", int baud = 115200);
    ~LinuxSerial() override;

    void begin(long baud) override;
    void end() override;
    int available() override;
    int read() override;
    size_t write(uint8_t byte) override;
    size_t write(const uint8_t* buf, size_t len) override;
    void flush() override;

private:
    int fd_ = -1;
    uint8_t tx_buf_[16];
    size_t tx_len_ = 0;
};
