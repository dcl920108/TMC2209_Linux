// #pragma once

// class TMC2209Driver {
// public:
//     TMC2209Driver() = default;
//     ~TMC2209Driver() = default;
// }; 3/23/2026

// #pragma once
// #include "linux_serial.h"
// #include "TMC2209.h"
// #include <cstdint>

// class TMC2209Driver {
// public:
//     TMC2209Driver(int step_pin, int dir_pin, int en_pin,
//                   const char* uart_dev = "/dev/ttyAMA2");
//     ~TMC2209Driver();

//     void configure(uint16_t current_ma, uint8_t microsteps);
//     void setEnabled(bool en);

// private:
//     LinuxSerial serial_;
//     TMC2209     stepper_;
//     int step_pin_;
//     int dir_pin_;
//     int en_pin_;
// }; 3/26/2026

// #pragma once
// #include "linux_serial.h"
// #include "TMC2209.h"
// #include <cstdint>

// class TMC2209Driver {
// public:
//     TMC2209Driver(int step_pin, int dir_pin, int en_pin,
//                   const char* uart_dev = "/dev/ttyAMA2");
//     ~TMC2209Driver();

//     void configure(uint16_t current_ma, uint8_t microsteps);
//     void setEnabled(bool en);
//     void stepPulse(uint32_t steps, bool dir);

//     uint32_t max_speed_   = 2000;
//     uint32_t accel_       = 500;
//     uint32_t start_speed_ = 100;

// private:
//     LinuxSerial serial_;
//     TMC2209     stepper_;
//     int step_pin_;
//     int dir_pin_;
//     int en_pin_;
// }; 4/6/2026

// include/tmc2209_driver.h
// #pragma once
// #include "linux_serial.h"
// #include "trapezoidal.h"
// #include "TMC2209.h"
// #include <cstdint>
// #include <atomic>

// class TMC2209Driver {
// public:
//     TMC2209Driver(int step_pin = 16, int dir_pin = 25, int en_pin = 5,
//                   const char* uart_dev = "/dev/ttyAMA2");
//     ~TMC2209Driver();

//     void configure(uint16_t current_ma, uint8_t microsteps);
//     void setEnabled(bool en);
//     void stepPulse(uint32_t steps, bool dir);
//     //4/9/2026
//     unit32_t runToPosition(uint32_t steps, bool dir);

//     // Week 5 — StallGuard
//     void setupStallGuard(uint8_t sgthrs, uint16_t tcoolthrs, uint16_t sw_threshold = 3);
//     bool isStalled() const;
//     void resetStall();

//     uint32_t max_speed_   = 2000;
//     uint32_t accel_       = 500;
//     uint32_t start_speed_ = 100;

// private:
//     LinuxSerial serial_;
//     TMC2209     stepper_;
//     int step_pin_;
//     int dir_pin_;
//     int en_pin_;
//     int diag_pin_ = 23;

//     // StallGuard state
//     std::atomic<int>  stall_count_{0};
//     std::atomic<bool> stall_detected_{false};
//     uint16_t sw_threshold_ = 3;

//     static void stallISR(int gpio, int level, uint32_t tick, void* userdata);
// }; 4/14/2026

#pragma once
#include "linux_serial.h"
#include "TMC2209.h"
#include <cstdint>
#include <atomic>

class TMC2209Driver {
public:
    TMC2209Driver(int step_pin, int dir_pin, int en_pin,
                  const char* uart_dev = "/dev/ttyAMA2");
    ~TMC2209Driver();

    void configure(uint16_t current_ma, uint8_t microsteps);
    void setEnabled(bool en);
    void stepPulse(uint32_t steps, bool dir);

    // Week 5 - StallGuard
    void setupStallGuard(uint8_t sgthrs, uint16_t tcoolthrs, uint16_t sw_threshold = 3);
    bool isStalled() const;
    void resetStall();
    uint16_t getStallGuardResult();

    // UART diagnostics (added 4/14 after UART bring-up)
    uint8_t getInterfaceCount();
    bool isCommunicating();
    uint8_t getVersion();

    uint32_t max_speed_   = 2000;
    uint32_t accel_       = 500;
    uint32_t start_speed_ = 100;

private:
    static void stallISR(int gpio, int level, uint32_t tick, void* userdata);

    LinuxSerial serial_;
    TMC2209     stepper_;
    int step_pin_;
    int dir_pin_;
    int en_pin_;

    int diag_pin_ = 23;
    uint16_t sw_threshold_ = 3;
    std::atomic<int>  stall_count_{0};
    std::atomic<bool> stall_detected_{false};
};
