# TMC2209_Linux

C++ TMC2209 stepper driver for Raspberry Pi Linux. Port of [janelia-arduino/TMC2209](https://github.com/janelia-arduino/TMC2209) — replacing Arduino `HardwareSerial` with Linux termios UART.

Includes StallGuard4 stall detection verified end-to-end on Raspberry Pi CM4, and a complete AGD (Alcohol Genotype Device) motor demo with KivyMD GUI integration.

---

## Why this driver?

Replaces the Python PyTmcStepper driver, which suffered from **GPIO23 resource conflicts** on consecutive runs:
- Each cycle required `GPIO.remove_event_detect(23)` workarounds
- Re-initialization was unreliable, frequently crashing after multiple runs
- Mixed RPi.GPIO + gpiozero libraries caused hard-to-debug conflicts

This C++ driver eliminates the conflict entirely:
- pigpio singleton properly shared across motor + TEC libraries (no `gpioTerminate()` in destructor)
- Clean ISR registration/release lifecycle
- StallGuard polling architecture (matches janelia reference)

**Verified: tested with multiple consecutive cycles via KivyMD button — no GPIO conflicts, no manual cleanup needed.**

---

## Hardware

- Raspberry Pi CM4
- TMC2209 stepper driver (SilentStepStick form factor)
- NEMA17 stepper motor
- UART: `/dev/ttyAMA2` (57600 baud, single-wire half-duplex)
- Step/Dir pulse generation via pigpio
- DIAG pin on GPIO 23 (StallGuard)
- Limit sensor on GPIO 22 (homing)

### GPIO pinout (BCM)

| Function | Pin |
|----------|-----|
| STEP     | 16  |
| DIR      | 25  |
| EN       | 5   |
| DIAG (StallGuard) | 23 |
| Limit sensor (homing) | 22 |
| UART | /dev/ttyAMA2 (GPIO 0/1) |

---

## Dependencies

- pigpio
- pybind11
- CMake >= 3.16
- g++ (C++17)

```
sudo apt install cmake g++ pigpio python3-pybind11
```

---

## Project Structure

```
TMC2209_Linux/
├── CMakeLists.txt
├── README.md
├── PORTING_NOTES.md              (Arduino → Linux porting notes)
├── Current_DEBUG_NOTES_0415.md   (StallGuard debugging history)
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
│   ├── tmc2209_driver.cpp (configure, setEnabled, stepPulse, StallGuard polling)
│   ├── trapezoidal.cpp    (trapezoidal acceleration algorithm)
│   └── bindings.cpp       (pybind11 Python bindings with GIL release)
├── agd_motor_demo.py             (KivyMD demo: pierce + homing full cycle)
├── test_*.py                     (standalone test scripts)
├── third_party/
│   └── janelia/
│       ├── TMC2209.h
│       └── TMC2209.cpp
└── archive/
    └── isr_architecture_v1/      (original ISR-based StallGuard, replaced in Week 7-8)
```

---

## Build

```
mkdir build && cd build
cmake ..
make
```

Produces `tmc2209_module.cpython-*.so` in `build/`.

---

## Python Usage — Basic Motion

```python
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

---

## StallGuard — Verified Working Configuration

StallGuard4 detection has been verified end-to-end on CM4 hardware. Motor stops reliably on manual stall and actual step count is returned for downstream position feedback.

### Setup (StallGuard parameters integrated into `configure()`)

```python
tmc.configure(current_ma=600, microsteps=1, sgthrs=70, tcoolthrs=0xFFFFF)
# SG threshold = sgthrs * 2 = 140
# TCOOLTHRS at full 20-bit range forces StallGuard active at all velocities
```

### Detection pattern — `step_pulse_until_triggered()`

The janelia library's native detection pattern is **polling SG_RESULT against SGTHRS\*2**, not interrupt-driven. The C++ driver implements this internally, with a configurable warmup window to suppress startup-phase noise:

```python
# Pierce-to-bottom: run up to max_steps, stop after 'threshold' consecutive triggers
steps = tmc.step_pulse_until_triggered(
    max_steps=12000,
    dir=True,
    threshold=20,        # consecutive trigger count to confirm stall
    warmup_steps=200     # ignore triggers in first N steps (motor accel noise)
)
print(f"Stopped at step {steps}")
```

Typical detection latency: a few milliseconds after stall onset (per-step polling in C++).

### Verified parameters (CM4 + pigpio + janelia)

| Parameter | Value | Notes |
|-----------|-------|-------|
| SGTHRS | 70 | Threshold = 140 |
| TCOOLTHRS | 0xFFFFF | Full 20-bit range |
| RMS current | 600 mA | `setRMSCurrent(600, 0.11, 0.9)` |
| R_sense | 0.11 Ω | AGD board sense resistor |
| Chopper mode | StealthChop | `enableStealthChop()` before `enable()` |
| CoolStep | enabled | SEMIN=1 does not suppress DIAG |
| Max speed | 500 Hz | AGD pierce config |
| Accel | 150 | AGD pierce config |
| Trigger threshold | 20 consecutive | Tuned for AGD needle insertion |
| Warmup steps | 200 | Suppresses startup acceleration noise |

---

## Python API surface

```python
# Init + config
tmc = TMC2209Driver(step_pin, dir_pin, en_pin, uart_dev='/dev/ttyAMA2')
tmc.configure(current_ma, microsteps, sgthrs=70, tcoolthrs=0xFFFFF)

# Motion parameters (read/write attributes)
tmc.max_speed = 500
tmc.accel = 150
tmc.start_speed = 100

# Enable / disable
tmc.set_enabled(True)
tmc.set_enabled(False)

# Motion
tmc.step_pulse(steps, direction)                                    # blocking N-step move
tmc.step_pulse_until_triggered(max_steps, dir, threshold, warmup)   # StallGuard-stopped move

# Diagnostics
tmc.get_stallguard_result()       # read live SG_RESULT
tmc.read_diag_level()             # read DIAG pin (0 or 1)
tmc.is_stealthchop_enabled()      # verify chopper mode
tmc.get_version()                 # expected 0x21
tmc.get_ifcnt()                   # UART interface counter
tmc.is_communicating()            # UART link health
```

---

## AGD Application Demo

`agd_motor_demo.py` is a complete KivyMD application demonstrating the full needle insertion cycle:

1. **Pierce to bottom** — `step_pulse_until_triggered()` with StallGuard polling
2. **Homing** — single-step reverse + GPIO22 limit sensor + back-off
3. **Multi-cycle validation** — proves the GPIO23 conflict from PyTmcStepper is eliminated

Run:
```
sudo python3 agd_motor_demo.py
```

---

## Notes on known suspects (ruled out on this hardware)

In the community thread that helped track StallGuard down, two suspects were flagged first but turned out to be non-blocking under janelia library defaults on this hardware:

- **GCONF.diag_stall** — janelia defaults are correct, no manual set required
- **COOLCONF.SEMIN=1** — CoolStep active does not suppress DIAG assertion

The parameter that matters most for first-time bring-up is **SGTHRS**. The back-EMF at your operating point (speed + current) sets a floor on how low SG_RESULT can be pulled under manual stall. Choose SGTHRS such that `SGTHRS*2` is above that floor.

---

## Architecture evolution

The current driver uses **polling-based StallGuard**, which replaces an earlier ISR-based architecture (Week 5 design). The original ISR implementation is preserved in `archive/isr_architecture_v1/` for reference.

Key insight from Week 7-8: the janelia-arduino library has no `isStalled()` API by design — the official pattern is polling `getStallGuardResult()` and comparing to `SGTHRS*2`. ISR registration on DIAG is not needed and adds unnecessary complexity.

---

## Development timeline

5-week sprint (Mar 9 – May 1, 2026):

| Week | Milestone |
|------|-----------|
| 1-3  | CMake skeleton, LinuxSerial UART, register init |
| 4    | Trapezoidal motion profile, first motor run |
| 5    | StallGuard ISR (later replaced) |
| 6    | UART debugging, current configuration fixes |
| 7    | StallGuard architecture pivot — ISR → polling |
| 8    | AGD demo integration, multi-cycle validation, GitHub release |

---

## Acknowledgments

- [janelia-arduino/TMC2209](https://github.com/janelia-arduino/TMC2209) — upstream library
- [Kletternaut/TMC2209_StallGuard](https://github.com/Kletternaut/TMC2209_StallGuard) — ESP32 reference firmware, provided the polling architecture insight
- Community contributors to janelia-arduino/TMC2209 issue #106 — back-EMF / SGTHRS diagnostic that unblocked detection

---

## License

BSD 3-Clause (based on janelia-arduino/TMC2209)
