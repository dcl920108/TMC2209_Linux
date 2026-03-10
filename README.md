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
│   ├── linux_serial.h
│   └── tmc2209_driver.h
├── src/
│   ├── linux_serial.cpp
│   ├── tmc2209_driver.cpp
│   ├── trapezoidal.cpp
│   └── bindings.cpp
└── third_party/
    └── janelia/
        ├── TMC2209.h      (from janelia-arduino/TMC2209 src/)
        └── TMC2209.cpp    (from janelia-arduino/TMC2209 src/TMC2209/)
```

## Build
```bash
mkdir build && cd build
cmake ..
make
```

## License

BSD 3-Clause (based on janelia-arduino/TMC2209)
