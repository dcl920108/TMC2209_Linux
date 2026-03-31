// #include "tmc2209_driver.h"
// 3/24/2026

// #include "tmc2209_driver.h"
// #include <pigpio.h>

// TMC2209Driver::TMC2209Driver(int step_pin, int dir_pin, int en_pin,
//                              const char* uart_dev)
//     : serial_(uart_dev)
//     , step_pin_(step_pin)
//     , dir_pin_(dir_pin)
//     , en_pin_(en_pin)
// {
//     gpioInitialise();
//     gpioSetMode(step_pin_, PI_OUTPUT);
//     gpioSetMode(dir_pin_,  PI_OUTPUT);
//     gpioSetMode(en_pin_,   PI_OUTPUT);
//     gpioWrite(en_pin_, 1);  // disabled by default (ENN is active-low)
// }

// TMC2209Driver::~TMC2209Driver() {
//     gpioWrite(en_pin_, 1);  // disable motor
//     // DO NOT call gpioTerminate() — TEC library shares pigpio
// }

// void TMC2209Driver::configure(uint16_t current_ma, uint8_t microsteps) {
//     stepper_.setup(serial_);
//     stepper_.setRunCurrent(current_ma);
//     stepper_.setMicrostepsPerStep(microsteps);
//     stepper_.enableCoolStep();
// }

// void TMC2209Driver::setEnabled(bool en) {
//     gpioWrite(en_pin_, en ? 0 : 1);  // active-low: 0 = enabled
// } 3/31/2026

#include "tmc2209_driver.h"
#include "trapezoidal.h"
#include <pigpio.h>
#include <algorithm>

TMC2209Driver::TMC2209Driver(int step_pin, int dir_pin, int en_pin,
                             const char* uart_dev)
    : serial_(uart_dev)
    , step_pin_(step_pin)
    , dir_pin_(dir_pin)
    , en_pin_(en_pin)
{
    gpioInitialise();
    gpioSetMode(step_pin_, PI_OUTPUT);
    gpioSetMode(dir_pin_,  PI_OUTPUT);
    gpioSetMode(en_pin_,   PI_OUTPUT);
    gpioWrite(en_pin_, 1);
}

TMC2209Driver::~TMC2209Driver() {
    gpioWrite(en_pin_, 1);
}

void TMC2209Driver::configure(uint16_t current_ma, uint8_t microsteps) {
    stepper_.setup(serial_);
    stepper_.setRunCurrent(current_ma);
    stepper_.setMicrostepsPerStep(microsteps);
    stepper_.enableCoolStep();
}

void TMC2209Driver::setEnabled(bool en) {
    gpioWrite(en_pin_, en ? 0 : 1);
}

void TMC2209Driver::stepPulse(uint32_t steps, bool dir) {
    if (steps == 0) return;

    gpioWrite(dir_pin_, dir ? 1 : 0);

    auto prof = computeProfile(steps, start_speed_, max_speed_, accel_);
    uint32_t v = start_speed_;

    for (uint32_t i = 0; i < steps; i++) {
        // determine current phase and adjust speed
        if (i < prof.accel_steps) {
            v = start_speed_ + (i + 1) * accel_;
            v = std::min(v, max_speed_);
        } else if (i >= prof.accel_steps + prof.const_steps) {
            uint32_t decel_i = i - prof.accel_steps - prof.const_steps;
            uint32_t v_peak = start_speed_ + prof.accel_steps * accel_;
            v_peak = std::min(v_peak, max_speed_);
            v = (v_peak > (decel_i + 1) * accel_) 
                ? v_peak - (decel_i + 1) * accel_ 
                : start_speed_;
            v = std::max(v, start_speed_);
        }
        // else: const phase, v stays at max_speed_

        // pulse delay: half-period in microseconds
        uint32_t half_us = 500000 / v;
        if (half_us < 1) half_us = 1;

        gpioWrite(step_pin_, 1);
        gpioDelay(half_us);
        gpioWrite(step_pin_, 0);
        gpioDelay(half_us);
    }
}
