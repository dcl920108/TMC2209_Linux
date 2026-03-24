// #include "tmc2209_driver.h"
// 3/24/2026

#include "tmc2209_driver.h"
#include <pigpio.h>

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
    gpioWrite(en_pin_, 1);  // disable motor
    // DO NOT call gpioTerminate() — TEC library shares pigpio
}

void TMC2209Driver::configure(uint16_t current_ma, uint8_t microsteps) {
    stepper_.setup(serial_);
    stepper_.setRunCurrent(current_ma);
    stepper_.setMicrostepsPerStep(microsteps);
    stepper_.enableCoolStep();
}

void TMC2209Driver::setEnabled(bool en) {
    gpioWrite(en_pin_, en ? 0 : 1);  // active-low: 0 = enabled
}
