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
│   ├── Arduino.h          (shim — Arduino type aliases for Linux)
│   ├── Stream.h           (shim — virtual destructor base class)
│   ├── HardwareSerial.h   (shim — serial interface, base for LinuxSerial)
│   ├── linux_serial.h     (LinuxSerial class, inherits HardwareSerial)
│   └── tmc2209_driver.h   (driver class declaration)
├── src/
│   ├── linux_serial.cpp   (full termios UART implementation)
│   ├── tmc2209_driver.cpp (driver implementation — Week 3+)
│   ├── trapezoidal.cpp    (acceleration algorithm — Week 4)
│   └── bindings.cpp       (pybind11 Python bindings)
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

typedef uint8_t byte;
typedef bool boolean;

inline void delay(unsigned long ms) {}
inline void delayMicroseconds(unsigned int us) {}
inline unsigned long millis() { return 0; }
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
// Week 3: constructor, configure(), setEnabled() implementation
```

### src/trapezoidal.cpp
```cpp
// Week 4: trapezoidal acceleration algorithm
```

### src/bindings.cpp
```cpp
#include <pybind11/pybind11.h>
#include "tmc2209_driver.h"

namespace py = pybind11;

PYBIND11_MODULE(tmc2209_module, m) {
    m.doc() = "TMC2209 Linux driver";
    py::class_<TMC2209Driver>(m, "TMC2209Driver")
        .def(py::init<>());
}
```

## Build
```bash
mkdir build && cd build
cmake ..
make
```

## Python Usage (Week 7+)
```python
from tmc2209_module import TMC2209Driver

tmc = TMC2209Driver(step_pin=21, dir_pin=20, en_pin=23)
tmc.configure(current_ma=800, microsteps=8)
tmc.run_to_position(3000)
# del tmc → destructor releases GPIO23 cleanly
```

## License

BSD 3-Clause (based on janelia-arduino/TMC2209)
