
# TMC2209_Linux

C++ TMC2209 stepper driver for Raspberry Pi Linux.
Port of janelia-arduino/TMC2209 — replacing Arduino HardwareSerial with Linux termios UART.

Includes StallGuard4 stall detection verified end-to-end on Raspberry Pi CM4.

## Hardware

- Raspberry Pi CM4
- TMC2209 stepper driver
- UART: `/dev/ttyAMA2` (57600 baud, single-wire half-duplex)
- Step/Dir pulse generation via pigpio
- DIAG pin on GPIO 23 (for StallGuard)

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
│   ├── tmc2209_driver.cpp (driver: configure, setEnabled, stepPulse, StallGuard)
│   ├── trapezoidal.cpp    (trapezoidal acceleration algorithm)
│   └── bindings.cpp       (pybind11 Python bindings with GIL release)
└── third_party/
    └── janelia/
        ├── TMC2209.h
        └── TMC2209.cpp
```

## Build

```
mkdir build && cd build
cmake ..
make
```

## Python Usage — Basic Motion

```
import sys
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
from tmc2209_module import TMC2209Driver

tmc = TMC2209Driver(step_pin=16, dir_pin=25, en_pin=5)
tmc.configure(current_ma=600, microsteps=1)
tmc.max_speed   = 1000
tmc.accel       = 500
tmc.start_speed = 100

tmc.set_enabled(True)
tmc.step_pulse(2000, True)   # forward
tmc.step_pulse(2000, False)  # reverse
tmc.set_enabled(False)

del tmc
```

## StallGuard — Verified Working Configuration

StallGuard4 detection has been verified end-to-end on CM4 hardware. Motor
stops reliably on manual stall and actual step count is returned for
downstream position feedback.

### Setup

```
tmc.setup_stallguard(sgthrs=70, tcoolthrs=0xFFFFF)
# SG threshold = sgthrs * 2 = 140
# TCOOLTHRS at full 20-bit range forces StallGuard active at all velocities
```

### Detection pattern — chunk-based polling

The janelia library's native detection pattern is polling `SG_RESULT`
against `SGTHRS * 2`, not interrupt-driven. Run the motor in small chunks
and read the DIAG pin level between chunks:

```
def step_until_stall(tmc, max_steps, direction, chunk_size=200):
    """Run up to max_steps, break on DIAG rising. Returns (actual_steps, stalled)."""
    total = 0
    while total < max_steps:
        this_chunk = min(chunk_size, max_steps - total)
        tmc.step_pulse(this_chunk, direction)
        total += this_chunk
        if tmc.read_diag_level() == 1:
            return (total, True)
    return (total, False)

# Usage
actual, stalled = step_until_stall(tmc, max_steps=3000, direction=True, chunk_size=200)
if stalled:
    print(f"Stalled at step {actual}")
```

Typical detection latency: ~100 ms at `chunk_size=200, max_speed=1000 Hz`.

### Verified parameters (CM4 + pigpio + janelia)

| Parameter      | Value          | Notes                                    |
|----------------|----------------|------------------------------------------|
| SGTHRS         | 70             | Threshold = 140                          |
| TCOOLTHRS      | 0xFFFFF        | Full 20-bit range                        |
| RMS current    | 600 mA         | `setRMSCurrent(600, 0.11, 0.9)`          |
| R_sense        | 0.11 Ω         | AGD board sense resistor                 |
| Chopper mode   | StealthChop    | `enableStealthChop()` before `enable()`  |
| CoolStep       | enabled        | `SEMIN=1` does not suppress DIAG         |
| Max speed      | 1000 Hz        |                                          |
| Chunk size     | 200 steps      | Detection latency ~100 ms                |

### Python API surface for StallGuard

```
tmc.setup_stallguard(sgthrs, tcoolthrs)    # configure threshold + velocity gate
tmc.get_sg_result()                         # read live SG_RESULT value
tmc.read_diag_level()                       # read DIAG pin (0 or 1)
tmc.is_stealthchop_enabled()                # verify chopper mode
tmc.get_version()                           # expected 0x21
```

### Notes on known suspects (ruled out on this hardware)

In the community thread that helped track this down, two suspects were
flagged first but turned out to be non-blocking under janelia library
defaults on this hardware:

- **`GCONF.diag_stall`** — janelia defaults are correct, no manual set required
- **`COOLCONF.SEMIN=1`** — CoolStep active does not suppress DIAG assertion

The parameter that matters most for first-time bring-up is **SGTHRS**. The
back-EMF at your operating point (speed + current) sets a floor on how low
SG_RESULT can be pulled under manual stall. Choose SGTHRS such that
`SGTHRS*2` is above that floor.

## Test Scripts (Not in Repo)

Hardware verification scripts are kept locally on the CM4 development host
rather than in the repo. Refer to `step_until_stall` pattern above for the
canonical usage.

## Acknowledgments

- [janelia-arduino/TMC2209](https://github.com/janelia-arduino/TMC2209) — upstream library
- [Kletternaut/TMC2209_StallGuard](https://github.com/Kletternaut/TMC2209_StallGuard) — ESP32 reference firmware, provided the polling architecture insight
- Community contributors to [janelia-arduino/TMC2209 issue #106](https://github.com/janelia-arduino/TMC2209/issues/106) — back-EMF / SGTHRS diagnostic that unblocked detection

## License

BSD 3-Clause (based on janelia-arduino/TMC2209)
