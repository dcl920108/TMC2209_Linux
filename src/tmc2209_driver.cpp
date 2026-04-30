#include "tmc2209_driver.h"
#include "trapezoidal.h"
#include <pigpio.h>
#include <algorithm>
#include <unistd.h>
#include <cstdio>

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
    gpioWrite(en_pin_, 1);
    // DO NOT call gpioTerminate() - TEC library shares pigpio
}

// configure() sequence:
//   1. setup()         - bind serial + read default registers
//   2. setRMSCurrent   - set IRUN (real mA)
//   3. setMicrosteps   - set MRES bit field
//   4. enableCoolStep  - turn on CoolStep current reduction
//   5. enableStealthChop - force StealthChop (required for StallGuard4)
//   6. setStallGuardThreshold + setCoolStepDurationThreshold
//   7. enable()        - CRITICAL: writes CHOPCONF.TOFF = toff_ (non-zero).
//                        Without this, TOFF stays 0 and chopper is disabled,
//                        so motor coils get no current and motor will not move.
//
// Each register write is followed by flush() + 5ms sleep to force an
// inter-frame idle gap.
void TMC2209Driver::configure(uint16_t current_ma, uint8_t microsteps,
                              uint8_t sgthrs, uint32_t tcoolthrs) {
    stepper_.setup(serial_);
    serial_.flush();
    usleep(5000);

    stepper_.setRMSCurrent(current_ma, 0.11f, 0.9f);
    serial_.flush();
    usleep(5000);

    stepper_.setMicrostepsPerStep(microsteps);
    serial_.flush();
    usleep(5000);

    stepper_.enableCoolStep();
    serial_.flush();
    usleep(5000);

    stepper_.enableStealthChop();   // force StealthChop before enable().
    serial_.flush();                // StealthChop only activates at standstill;
    usleep(5000);                   // explicit call guarantees the mode StallGuard4 requires.

    stepper_.setStallGuardThreshold(sgthrs);
    serial_.flush();
    usleep(5000);

    stepper_.setCoolStepDurationThreshold(tcoolthrs);
    serial_.flush();
    usleep(5000);

    stepper_.enable();
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

uint32_t TMC2209Driver::stepPulseUntilTriggered(uint32_t max_steps, bool dir, uint32_t threshold,
                                                 uint32_t warmup_steps) {
    if (max_steps == 0) return 0;

    gpioWrite(dir_pin_, dir ? 1 : 0);

    auto prof = computeProfile(max_steps, start_speed_, max_speed_, accel_);
    uint32_t v = start_speed_;
    uint32_t trigger_count = 0;
    int last_diag = 0;

    for (uint32_t i = 0; i < max_steps; i++) {
        // Speed profile (same as stepPulse)
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

        // Check DIAG - count only LOW->HIGH edges, not held HIGH state
        int now_diag = gpioRead(diag_pin_);
        if (now_diag == 1 && last_diag == 0 && i >= warmup_steps) {
            trigger_count++;
            printf("[C++] trigger #%u at step %u\n", trigger_count, i + 1);
            fflush(stdout);
            if (trigger_count >= threshold) {
                return i + 1;   // Return actual steps walked
            }
        }
        last_diag = now_diag;
    }

    return max_steps;   // Completed all steps without hitting threshold
}

// --- UART diagnostics ---

uint8_t TMC2209Driver::getInterfaceCount() {
    return stepper_.getInterfaceTransmissionCounter();
}

bool TMC2209Driver::isCommunicating() {
    return stepper_.isSetupAndCommunicating();
}

uint8_t TMC2209Driver::getVersion() {
    return stepper_.getVersion();
}

int TMC2209Driver::readDiagLevel() const {
    return gpioRead(diag_pin_);
}

bool TMC2209Driver::isStealthChopEnabled() {
    return stepper_.getSettings().stealth_chop_enabled;
}
