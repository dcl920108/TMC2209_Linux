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
│   ├── linux_serial.h     (LinuxSerial class declaration)
│   └── tmc2209_driver.h   (stub — Week 3+)
├── src/
│   ├── linux_serial.cpp   (full termios UART implementation)
│   ├── tmc2209_driver.cpp (stub — Week 3+)
│   ├── trapezoidal.cpp    (stub — Week 4)
│   └── bindings.cpp       (pybind11 module skeleton)
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

## Stub Files (Week 1)

### include/linux_serial.h
```cpp
#pragma once
#include <cstdint>
#include <cstddef>

class LinuxSerial {
public:
    LinuxSerial() = default;
    ~LinuxSerial() = default;
};
```

### include/tmc2209_driver.h
```cpp
#pragma once

class TMC2209Driver {
public:
    TMC2209Driver() = default;
    ~TMC2209Driver() = default;
};
```

### src/linux_serial.cpp
```cpp
#include "linux_serial.h"
```

### src/tmc2209_driver.cpp
```cpp
#include "tmc2209_driver.h"
```

### src/trapezoidal.cpp
```cpp
// placeholder
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

## License

BSD 3-Clause (based on janelia-arduino/TMC2209)
