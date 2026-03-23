// #pragma once

// class TMC2209Driver {
// public:
//     TMC2209Driver() = default;
//     ~TMC2209Driver() = default;
// }; 3/23/2026

#pragma once
#include "linux_serial.h"
#include "TMC2209.h"
#include <cstdint>

class TMC2209Driver {
public:
    TMC2209Driver(int step_pin, int dir_pin, int en_pin,
                  const char* uart_dev = "/dev/ttyAMA2");
    ~TMC2209Driver();

    void configure(uint16_t current_ma, uint8_t microsteps);
    void setEnabled(bool en);

private:
    LinuxSerial serial_;
    TMC2209     stepper_;
    int step_pin_;
    int dir_pin_;
    int en_pin_;
};
