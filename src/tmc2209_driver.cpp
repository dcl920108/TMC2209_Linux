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

// #include "tmc2209_driver.h"
// #include "trapezoidal.h"
// #include <pigpio.h>
// #include <algorithm>

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
//     gpioWrite(en_pin_, 1);
// }

// TMC2209Driver::~TMC2209Driver() {
//     gpioWrite(en_pin_, 1);
// }

// void TMC2209Driver::configure(uint16_t current_ma, uint8_t microsteps) {
//     stepper_.setup(serial_);
//     stepper_.setRunCurrent(current_ma);
//     stepper_.setMicrostepsPerStep(microsteps);
//     stepper_.enableCoolStep();
// }

// void TMC2209Driver::setEnabled(bool en) {
//     gpioWrite(en_pin_, en ? 0 : 1);
// }

// void TMC2209Driver::stepPulse(uint32_t steps, bool dir) {
//     if (steps == 0) return;

//     gpioWrite(dir_pin_, dir ? 1 : 0);

//     auto prof = computeProfile(steps, start_speed_, max_speed_, accel_);
//     uint32_t v = start_speed_;

//     for (uint32_t i = 0; i < steps; i++) {
//         // determine current phase and adjust speed
//         if (i < prof.accel_steps) {
//             v = start_speed_ + (i + 1) * accel_;
//             v = std::min(v, max_speed_);
//         } else if (i >= prof.accel_steps + prof.const_steps) {
//             uint32_t decel_i = i - prof.accel_steps - prof.const_steps;
//             uint32_t v_peak = start_speed_ + prof.accel_steps * accel_;
//             v_peak = std::min(v_peak, max_speed_);
//             v = (v_peak > (decel_i + 1) * accel_) 
//                 ? v_peak - (decel_i + 1) * accel_ 
//                 : start_speed_;
//             v = std::max(v, start_speed_);
//         }
//         // else: const phase, v stays at max_speed_

//         // pulse delay: half-period in microseconds
//         uint32_t half_us = 500000 / v;
//         if (half_us < 1) half_us = 1;

//         gpioWrite(step_pin_, 1);
//         gpioDelay(half_us);
//         gpioWrite(step_pin_, 0);
//         gpioDelay(half_us);
//     }
// } 4/6/2026

// src/tmc2209_driver.cpp
// #include "tmc2209_driver.h"
// #include <pigpio.h>
// #include <algorithm>

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
//     gpioSetISRFuncEx(diag_pin_, RISING_EDGE, 0, nullptr, nullptr);
//     gpioWrite(en_pin_, 1);
//     // DO NOT call gpioTerminate() — TEC library shares pigpio
// }

// void TMC2209Driver::configure(uint16_t current_ma, uint8_t microsteps) {
//     stepper_.setup(serial_);
//     stepper_.setRunCurrent(current_ma);
//     stepper_.setMicrostepsPerStep(microsteps);
//     stepper_.enableCoolStep();
// }

// void TMC2209Driver::setEnabled(bool en) {
//     gpioWrite(en_pin_, en ? 0 : 1);
// }

// void TMC2209Driver::stepPulse(uint32_t steps, bool dir) {
//     if (steps == 0) return;

//     gpioWrite(dir_pin_, dir ? 1 : 0);

//     auto prof = computeProfile(steps, start_speed_, max_speed_, accel_);
//     uint32_t v = start_speed_;

//     for (uint32_t i = 0; i < steps; i++) {
//         if (i < prof.accel_steps) {
//             v = start_speed_ + (i + 1) * accel_;
//             v = std::min(v, max_speed_);
//         } else if (i >= prof.accel_steps + prof.const_steps) {
//             uint32_t decel_i = i - prof.accel_steps - prof.const_steps;
//             uint32_t v_peak = start_speed_ + prof.accel_steps * accel_;
//             v_peak = std::min(v_peak, max_speed_);
//             v = (v_peak > (decel_i + 1) * accel_)
//                 ? v_peak - (decel_i + 1) * accel_
//                 : start_speed_;
//             v = std::max(v, start_speed_);
//         }

//         uint32_t half_us = 500000 / v;
//         if (half_us < 1) half_us = 1;

//         gpioWrite(step_pin_, 1);
//         gpioDelay(half_us);
//         gpioWrite(step_pin_, 0);
//         gpioDelay(half_us);
//     }
// }

// // --- Week 5: StallGuard ---

// void TMC2209Driver::setupStallGuard(uint8_t sgthrs, uint16_t tcoolthrs,
//                                      uint16_t sw_threshold) {
//     sw_threshold_ = sw_threshold;
//     stall_count_ = 0;
//     stall_detected_ = false;

//     stepper_.setStallGuardThreshold(sgthrs);
//     stepper_.setCoolStepDurationThreshold(tcoolthrs);

//     gpioSetISRFuncEx(diag_pin_, RISING_EDGE, 0, stallISR, this);
// }

// bool TMC2209Driver::isStalled() const {
//     return stall_detected_.load();
// }

// void TMC2209Driver::resetStall() {
//     stall_count_ = 0;
//     stall_detected_ = false;
// }

// void TMC2209Driver::stallISR(int gpio, int level, uint32_t tick, void* userdata) {
//     // placeholder — tomorrow's implementation
//     (void)gpio; (void)level; (void)tick; (void)userdata;
// } 4/8/2026

// #include "tmc2209_driver.h"
// #include <pigpio.h>
// #include <algorithm>

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
//     gpioSetISRFuncEx(diag_pin_, RISING_EDGE, 0, nullptr, nullptr);
//     gpioWrite(en_pin_, 1);
//     // DO NOT call gpioTerminate() — TEC library shares pigpio
// }

// void TMC2209Driver::configure(uint16_t current_ma, uint8_t microsteps) {
//     stepper_.setup(serial_);
//     stepper_.setRunCurrent(current_ma);
//     stepper_.setMicrostepsPerStep(microsteps);
//     stepper_.enableCoolStep();
// }

// void TMC2209Driver::setEnabled(bool en) {
//     gpioWrite(en_pin_, en ? 0 : 1);
// }

// void TMC2209Driver::stepPulse(uint32_t steps, bool dir) {
//     if (steps == 0) return;

//     gpioWrite(dir_pin_, dir ? 1 : 0);

//     auto prof = computeProfile(steps, start_speed_, max_speed_, accel_);
//     uint32_t v = start_speed_;

//     for (uint32_t i = 0; i < steps; i++) {
//         if (i < prof.accel_steps) {
//             v = start_speed_ + (i + 1) * accel_;
//             v = std::min(v, max_speed_);
//         } else if (i >= prof.accel_steps + prof.const_steps) {
//             uint32_t decel_i = i - prof.accel_steps - prof.const_steps;
//             uint32_t v_peak = start_speed_ + prof.accel_steps * accel_;
//             v_peak = std::min(v_peak, max_speed_);
//             v = (v_peak > (decel_i + 1) * accel_)
//                 ? v_peak - (decel_i + 1) * accel_
//                 : start_speed_;
//             v = std::max(v, start_speed_);
//         }

//         uint32_t half_us = 500000 / v;
//         if (half_us < 1) half_us = 1;

//         gpioWrite(step_pin_, 1);
//         gpioDelay(half_us);
//         gpioWrite(step_pin_, 0);
//         gpioDelay(half_us);
//     }
// }

// //4/9/2026
// uint32_t TMC2209Driver::runToPosition(uint32_t steps, bool dir) {
//     if (steps == 0) return 0;

//     resetStall();
//     gpioWrite(dir_pin_, dir ? 1 : 0);

//     auto prof = computeProfile(steps, start_speed_, max_speed_, accel_);
//     uint32_t v = start_speed_;
//     uint32_t i = 0;

//     for (i = 0; i < steps; i++) {
//         if (isStalled()) {
//             break;
//         }

//         if (i < prof.accel_steps) {
//             v = start_speed_ + (i + 1) * accel_;
//             v = std::min(v, max_speed_);
//         } else if (i >= prof.accel_steps + prof.const_steps) {
//             uint32_t decel_i = i - prof.accel_steps - prof.const_steps;
//             uint32_t v_peak = start_speed_ + prof.accel_steps * accel_;
//             v_peak = std::min(v_peak, max_speed_);
//             v = (v_peak > (decel_i + 1) * accel_)
//                 ? v_peak - (decel_i + 1) * accel_
//                 : start_speed_;
//             v = std::max(v, start_speed_);
//         }

//         uint32_t half_us = 500000 / v;
//         if (half_us < 1) half_us = 1;

//         gpioWrite(step_pin_, 1);
//         gpioDelay(half_us);
//         gpioWrite(step_pin_, 0);
//         gpioDelay(half_us);
//     }

//     return i;
// }

// // --- Week 5: StallGuard ---

// void TMC2209Driver::setupStallGuard(uint8_t sgthrs, uint16_t tcoolthrs,
//                                      uint16_t sw_threshold) {
//     sw_threshold_ = sw_threshold;
//     stall_count_ = 0;
//     stall_detected_ = false;

//     stepper_.setStallGuardThreshold(sgthrs);
//     stepper_.setCoolStepDurationThreshold(tcoolthrs);

//     gpioSetISRFuncEx(diag_pin_, RISING_EDGE, 0, stallISR, this);
// }

// bool TMC2209Driver::isStalled() const {
//     return stall_detected_.load(std::memory_order_acquire);
// }

// void TMC2209Driver::resetStall() {
//     stall_count_ = 0;
//     stall_detected_ = false;
// }

// void TMC2209Driver::stallISR(int gpio, int level, uint32_t tick, void* userdata) {
//     (void)gpio;
//     (void)level;
//     (void)tick;

//     TMC2209Driver* self = static_cast<TMC2209Driver*>(userdata);
//     if (self == nullptr) return;

//     self->stall_count_.fetch_add(1, std::memory_order_relaxed);

//     if (self->stall_count_.load(std::memory_order_relaxed) >= self->sw_threshold_) {
//         self->stall_detected_.store(true, std::memory_order_release);
//     }
// } 4/14/2026

#include "tmc2209_driver.h"
#include "trapezoidal.h"
#include <pigpio.h>
#include <algorithm>
#include <unistd.h>

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
    gpioWrite(en_pin_, 1);  // disabled by default (ENN is active-low)
}

TMC2209Driver::~TMC2209Driver() {
    gpioSetISRFuncEx(diag_pin_, RISING_EDGE, 0, nullptr, nullptr);
    gpioWrite(en_pin_, 1);
    // DO NOT call gpioTerminate() - TEC library shares pigpio
}

// Each register write is followed by flush() + 5ms sleep to force an
// inter-frame idle gap. Without this, the buffered-write mechanism in
// LinuxSerial causes multiple datagrams to be emitted back-to-back with
// no idle between them, which violates the TMC2209 UART framing rules
// and causes the chip to stop responding.
void TMC2209Driver::configure(uint16_t current_ma, uint8_t microsteps) {
    stepper_.setup(serial_);
    serial_.flush();
    usleep(5000);

    stepper_.setRunCurrent(current_ma);
    serial_.flush();
    usleep(5000);

    stepper_.setMicrostepsPerStep(microsteps);
    serial_.flush();
    usleep(5000);

    stepper_.enableCoolStep();
    serial_.flush();
    usleep(5000);
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

        uint32_t half_us = 500000 / v;
        if (half_us < 1) half_us = 1;

        gpioWrite(step_pin_, 1);
        gpioDelay(half_us);
        gpioWrite(step_pin_, 0);
        gpioDelay(half_us);
    }
}

// --- Week 5: StallGuard ---

void TMC2209Driver::setupStallGuard(uint8_t sgthrs, uint16_t tcoolthrs,
                                     uint16_t sw_threshold) {
    sw_threshold_ = sw_threshold;
    stall_count_ = 0;
    stall_detected_ = false;

    stepper_.setStallGuardThreshold(sgthrs);
    stepper_.setCoolStepDurationThreshold(tcoolthrs);

    gpioSetISRFuncEx(diag_pin_, RISING_EDGE, 0, stallISR, this);
}

bool TMC2209Driver::isStalled() const {
    return stall_detected_.load(std::memory_order_acquire);
}

void TMC2209Driver::resetStall() {
    stall_count_ = 0;
    stall_detected_ = false;
}

void TMC2209Driver::stallISR(int gpio, int level, uint32_t tick, void* userdata) {
    (void)gpio;
    (void)level;
    (void)tick;

    TMC2209Driver* self = static_cast<TMC2209Driver*>(userdata);
    if (self == nullptr) return;

    self->stall_count_.fetch_add(1, std::memory_order_relaxed);

    if (self->stall_count_.load(std::memory_order_relaxed) >= self->sw_threshold_) {
        self->stall_detected_.store(true, std::memory_order_release);
    }
}

uint16_t TMC2209Driver::getStallGuardResult() {
    return stepper_.getStallGuardResult();
}

// UART diagnostics (added 4/14 after UART bring-up)

uint8_t TMC2209Driver::getInterfaceCount() {
    return stepper_.getInterfaceTransmissionCounter();
}

bool TMC2209Driver::isCommunicating() {
    return stepper_.isSetupAndCommunicating();
}

uint8_t TMC2209Driver::getVersion() {
    return stepper_.getVersion();
}
