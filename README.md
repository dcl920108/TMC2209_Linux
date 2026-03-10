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

## Build
```bash
cd build
cmake ..
make
```

## License

BSD 3-Clause (based on janelia-arduino/TMC2209)
