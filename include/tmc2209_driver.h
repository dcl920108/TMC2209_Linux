#pragma once
#include "linux_serial.h"
#include "trapezoidal.h"
#include "TMC2209.h"
#include <cstdint>

class TMC2209Driver {
public:
    TMC2209Driver(int step_pin = 16, int dir_pin = 25, int en_pin = 5,
                  const char* uart_dev = "/dev/ttyAMA2");
    ~TMC2209Driver();

    void configure(uint16_t current_ma, uint8_t microsteps,
                   uint8_t sgthrs = 70, uint32_t tcoolthrs = 0xFFFFF);
    void setEnabled(bool en);
    void stepPulse(uint32_t steps, bool dir);
    uint32_t stepPulseUntilTriggered(uint32_t max_steps, bool dir, uint32_t threshold,
                                      uint32_t warmup_steps = 200);

    // UART diagnostics
    uint8_t getInterfaceCount();
    bool isCommunicating();

    // DIAG pin level read (pigpio direct)
    int readDiagLevel() const;

    // Chopper mode diagnostic
    bool isStealthChopEnabled();
    uint8_t getVersion();

    uint32_t max_speed_   = 2000;
    uint32_t accel_       = 500;
    uint32_t start_speed_ = 100;

private:
    LinuxSerial serial_;
    TMC2209     stepper_;
    int step_pin_;
    int dir_pin_;
    int en_pin_;
    int diag_pin_ = 23;   // Used by stepPulseUntilTriggered for polling DIAG edge
};
