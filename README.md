对，整个替换：

```markdown
# TMC2209_Linux

C++ TMC2209 stepper driver for Raspberry Pi Linux.
Port of janelia-arduino/TMC2209 — replacing Arduino HardwareSerial with Linux termios UART.

## Hardware

- Raspberry Pi CM4
- TMC2209 stepper driver
- UART: /dev/ttyAMA2 (57600 baud, single-wire half-duplex)
- Step/Dir pulse generation via pigpio

## Dependencies

- pigpio
- pybind11
- CMake >= 3.16
- g++ (C++17)

## Project Structure
```
TMC2209_Linux/
├── CMakeLists.txt
├── include/
│   ├── Arduino.h          (shim — Arduino types, macros, stub functions)
│   ├── Stream.h           (shim — virtual destructor base class)
│   ├── HardwareSerial.h   (shim — serial interface, base for LinuxSerial)
│   ├── SoftwareSerial.h   (shim — minimal stub for janelia compatibility)
│   ├── linux_serial.h     (LinuxSerial class, inherits HardwareSerial)
│   ├── tmc2209_driver.h   (driver class declaration)
│   └── trapezoidal.h      (TrapProfile struct + computeProfile declaration)
├── src/
│   ├── linux_serial.cpp   (full termios UART implementation)
│   ├── tmc2209_driver.cpp (driver: configure, setEnabled, stepPulse)
│   ├── trapezoidal.cpp    (trapezoidal acceleration algorithm)
│   └── bindings.cpp       (pybind11 Python bindings with GIL release)
└── third_party/
    └── janelia/
        ├── TMC2209.h      (from janelia-arduino/TMC2209 src/)
        └── TMC2209.cpp    (from janelia-arduino/TMC2209 src/TMC2209/)
```

## CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.16)
project(tmc2209_linux)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 REQUIRED)

pybind11_add_module(tmc2209_module
    src/linux_serial.cpp
    src/tmc2209_driver.cpp
    src/trapezoidal.cpp
    src/bindings.cpp
    third_party/janelia/TMC2209.cpp
)

target_include_directories(tmc2209_module PRIVATE
    include/
    third_party/janelia/
)

target_link_libraries(tmc2209_module PRIVATE pigpio pthread)
```

## Arduino Shim Headers

janelia source requires Arduino headers. These shims map Arduino types to Linux equivalents.

### include/Arduino.h
```cpp
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>

typedef uint8_t byte;
typedef bool boolean;

#define HIGH 1
#define LOW  0
#define OUTPUT 1
#define INPUT  0

inline void delay(unsigned long ms) {}
inline void delayMicroseconds(unsigned int us) {}
inline unsigned long millis() { return 0; }
inline void pinMode(uint8_t pin, uint8_t mode) {}
inline void digitalWrite(uint8_t pin, uint8_t val) {}
inline int digitalRead(uint8_t pin) { return 0; }

inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline long constrain(long x, long a, long b) {
    return std::min(std::max(x, a), b);
}

#include "HardwareSerial.h"
```

### include/Stream.h
```cpp
#pragma once
#include <cstdint>

class Stream {
public:
    virtual ~Stream() = default;
};
```

### include/HardwareSerial.h
```cpp
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
```

### include/SoftwareSerial.h
```cpp
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
```

## LinuxSerial (UART Communication Layer)

### include/linux_serial.h
```cpp
#pragma once
#include "HardwareSerial.h"
#include <cstdint>
#include <cstddef>

class LinuxSerial : public HardwareSerial {
public:
    explicit LinuxSerial(const char* port = "/dev/ttyAMA2", int baud = 57600);
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
};
```

### src/linux_serial.cpp
```cpp
#include "linux_serial.h"
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>

LinuxSerial::LinuxSerial(const char* port, int /*baud*/) {
    fd_ = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) return;
    struct termios tty{};
    tcgetattr(fd_, &tty);
    cfsetispeed(&tty, B57600);
    cfsetospeed(&tty, B57600);
    tty.c_cflag  = CS8 | CLOCAL | CREAD;
    tty.c_iflag  = IGNPAR;
    tty.c_oflag  = 0;
    tty.c_lflag  = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;
    tcflush(fd_, TCIOFLUSH);
    tcsetattr(fd_, TCSANOW, &tty);
}

LinuxSerial::~LinuxSerial() {
    if (fd_ >= 0) close(fd_);
}

void LinuxSerial::begin(long /*baud*/) {}

void LinuxSerial::end() {
    if (fd_ >= 0) { close(fd_); fd_ = -1; }
}

size_t LinuxSerial::write(uint8_t byte) {
    return write(&byte, 1);
}

size_t LinuxSerial::write(const uint8_t* buf, size_t len) {
    ssize_t n = ::write(fd_, buf, len);
    tcdrain(fd_);
    uint8_t echo[64];
    usleep(len * 200);
    ::read(fd_, echo, len);
    return (n > 0) ? n : 0;
}

int LinuxSerial::read() {
    uint8_t buf;
    fd_set rfds; FD_ZERO(&rfds); FD_SET(fd_, &rfds);
    struct timeval tv{0, 20000};
    if (select(fd_ + 1, &rfds, nullptr, nullptr, &tv) <= 0) return -1;
    return (::read(fd_, &buf, 1) == 1) ? buf : -1;
}

int LinuxSerial::available() {
    int n = 0;
    ioctl(fd_, FIONREAD, &n);
    return n;
}

void LinuxSerial::flush() {
    tcdrain(fd_);
}
```

## TMC2209 Driver (Main Class)

### include/tmc2209_driver.h
```cpp
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
    void stepPulse(uint32_t steps, bool dir);

    uint32_t max_speed_   = 2000;
    uint32_t accel_       = 500;
    uint32_t start_speed_ = 100;

private:
    LinuxSerial serial_;
    TMC2209     stepper_;
    int step_pin_;
    int dir_pin_;
    int en_pin_;
};
```

### src/tmc2209_driver.cpp
```cpp
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
```

## Trapezoidal Acceleration

### include/trapezoidal.h
```cpp
#pragma once
#include <cstdint>

struct TrapProfile {
    uint32_t accel_steps;
    uint32_t const_steps;
    uint32_t decel_steps;
};

TrapProfile computeProfile(uint32_t total_steps,
                           uint32_t start_speed,
                           uint32_t max_speed,
                           uint32_t accel);
```

### src/trapezoidal.cpp
```cpp
#include "trapezoidal.h"
#include <algorithm>

TrapProfile computeProfile(uint32_t total_steps,
                           uint32_t start_speed,
                           uint32_t max_speed,
                           uint32_t accel)
{
    if (accel == 0 || max_speed <= start_speed) {
        return {0, total_steps, 0};
    }

    uint32_t steps_to_max = (max_speed - start_speed) / accel;

    if (2 * steps_to_max >= total_steps) {
        uint32_t half = total_steps / 2;
        return {half, 0, total_steps - half};
    }

    uint32_t const_steps = total_steps - 2 * steps_to_max;
    return {steps_to_max, const_steps, steps_to_max};
}
```

## pybind11 Bindings

### src/bindings.cpp
```cpp
#include <pybind11/pybind11.h>
#include "tmc2209_driver.h"

namespace py = pybind11;

PYBIND11_MODULE(tmc2209_module, m) {
    m.doc() = "TMC2209 Linux driver";
    py::class_<TMC2209Driver>(m, "TMC2209Driver")
        .def(py::init<int, int, int, const char*>(),
             py::arg("step_pin"),
             py::arg("dir_pin"),
             py::arg("en_pin"),
             py::arg("uart_dev") = "/dev/ttyAMA2")
        .def("configure", &TMC2209Driver::configure,
             py::arg("current_ma"),
             py::arg("microsteps"))
        .def("set_enabled", &TMC2209Driver::setEnabled,
             py::arg("en"))
        .def("step_pulse", [](TMC2209Driver& self, uint32_t steps, bool dir) {
            py::gil_scoped_release release;
            self.stepPulse(steps, dir);
        }, py::arg("steps"), py::arg("dir"))
        .def_readwrite("max_speed", &TMC2209Driver::max_speed_)
        .def_readwrite("accel", &TMC2209Driver::accel_)
        .def_readwrite("start_speed", &TMC2209Driver::start_speed_);
}
```

## Build
```bash
mkdir build && cd build
cmake ..
make
```

## Python Usage
```python
import sys
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
from tmc2209_module import TMC2209Driver

tmc = TMC2209Driver(step_pin=16, dir_pin=25, en_pin=5)
tmc.max_speed = 500
tmc.accel = 200
tmc.start_speed = 50
tmc.set_enabled(True)

tmc.step_pulse(2000, True)   # forward
tmc.step_pulse(2000, False)  # reverse

tmc.set_enabled(False)
del tmc
```

## License

BSD 3-Clause (based on janelia-arduino/TMC2209)
```

Python Usage 部分也更新了——用的是实际验证通过的引脚号（16/25/5）和参数。
