# 4/15 Debug Notes — Current Configuration & Motor Enable

## Today's Outcome

- Motor running correctly under C++ driver
- Stage 2 (no-load false-trigger test) PASS
- TMC2209 + CM4 temperatures stable

## Two Critical Bugs Fixed Today

### Bug 1: setRunCurrent() takes PERCENT, not milliamps

The trap: janelia's API has the same name as PyTmcStepper's, but completely different semantics.

| Library | API | Parameter |
|---------|-----|-----------|
| PyTmcStepper | `set_current(run_current=600, ...)` | milliamps |
| janelia | `setRunCurrent(percent)` | percent (0-100) |

When we wrote `setRunCurrent(600)`:

- janelia internally calls `constrain(600, 0, 100)` → clamps to 100
- Then `map(100, 0, 100, 0, 31)` → IRUN bit field = 31 (full scale)
- Result: motor driven at full current capability of the board

Why we didn't notice immediately: the motor still ran (overdriven, but ran). Side effects accumulated as heat — TMC2209 hot, CM4 hot, screen blanking from power rail collapse.

### Bug 2: Missing stepper_.enable() call

The trap: TMC2209 has TWO enable layers. We were only doing one.

| Layer | What | We did? |
|-------|------|---------|
| Hardware EN pin | `gpioWrite(en_pin, 0)` | yes |
| Software CHOPCONF.TOFF | `stepper_.enable()` writes TOFF=toff_ | no |

On reset, TMC2209's `CHOPCONF.TOFF = 0`, which means chopper completely disabled — no current to motor coils, motor cannot move regardless of EN pin state or IRUN setting.

janelia's `enable()` source (line 113 of TMC2209.cpp):

```cpp
void TMC2209::enable() {
    if (hardware_enable_pin_ >= 0) {
        digitalWrite(hardware_enable_pin_, LOW);   // pulls EN low
    }
    chopper_config_.toff = toff_;                  // *** THE KEY LINE ***
    writeStoredChopperConfig();                    // writes CHOPCONF
}
```

The official janelia example always calls `stepper_driver.enable()` after `enableCoolStep()`. We missed this because the README/docs don't emphasize it as a required step.

## Final Working configure() Sequence

```cpp
void TMC2209Driver::configure(uint16_t current_ma, uint8_t microsteps) {
    stepper_.setup(serial_);              // bind UART, read defaults
    serial_.flush(); usleep(5000);

    stepper_.setRunCurrent(current_ma);   // NOTE: this is PERCENT (0-100)
    serial_.flush(); usleep(5000);

    stepper_.setMicrostepsPerStep(microsteps);
    serial_.flush(); usleep(5000);

    stepper_.enableCoolStep();
    serial_.flush(); usleep(5000);

    stepper_.enable();                    // *** REQUIRED — sets TOFF ***
    serial_.flush(); usleep(5000);
}
```

## Current Settings That Work

```python
tmc.configure(current_ma=600, microsteps=1)   # 600 here = 100% (max)
tmc.max_speed   = 1000
tmc.accel       = 500
tmc.start_speed = 100
```

Even though 600 clamps to 100% (= IRUN=31), the actual current to the motor is bounded by:

- TMC2209 vsense bit (default 0 = 325mV reference)
- BIGTREETECH v1.3 board Rsense = 0.11Ω
- TMC2209 internal overcurrent protection

In practice with the current motor + Rsense + vsense=0, this is hot but not destructive.

**TODO**: switch to `setRMSCurrent(mA, rSense, holdMul)` to get true mA-based control matching old PyTmcStepper config.

## Microstep Choice

| Setting | Use case |
|---------|----------|
| 1 (full step) | StallGuard tuning — datasheet recommends, SG_RESULT most accurate |
| 16 | Smooth motion in production |
| 32-256 | Very fine positioning (rarely needed for AGD needle insertion) |

Currently using `microsteps=1` for StallGuard testing. Switch to 16 for production motion later.

## Speed Math (microsteps=1)

```
max_speed = 1000 steps/s
200-step motor → 1000 / 200 = 5 rev/s = 300 RPM
```

Reasonable for needle insertion / homing operations.

## Why "Disconnect VM Power" Matters

TMC2209 internal registers (CHOPCONF, IHOLD_IRUN, COOLCONF, etc.) only reset when VM power (motor power supply) is removed. **`sudo reboot` does NOT reset them** — only the CM4 reboots, TMC2209 keeps last register state.

If a previous run wrote bad register values, you'll inherit them on the next run unless you cycle VM power.

## Temperature Status (4/15 end of day)

After fixes applied:

- TMC2209: warm but not hot
- CM4: normal idle temperature
- Motor: warm

The earlier overheating (CM4 hot + screen blanking) was likely caused by repeated test cycles with IRUN=31 + various GPIO state combinations stressing the 5V rail.

## Files Modified Today

- `src/tmc2209_driver.cpp` — added `stepper_.enable()` to configure()
- `test_configure_pulse.py` — microsteps 16→1, max_speed 500→1000
- `test_sg_stage2.py` — same speed/microstep changes

## What's Next (Tomorrow / Stage 3)

- Real-time SG_RESULT polling while motor runs
- Test with finger load on shaft to see SG_RESULT drop
- Find appropriate SGTHRS for AGD needle insertion scenario
- **Open question:** SGTHRS=100 was set but `is_stalled()` didn't trigger even though SG_RESULT=150 < 2*SGTHRS=200. Need to verify DIAG ISR path is actually firing (may need oscilloscope on DIAG pin or add ISR debug print).

## Key Lesson

Don't trust API names across libraries. PyTmcStepper and janelia both have `setRunCurrent` / `set_current` but the parameter semantics are completely different. Always read the source of any API touching hardware before passing values.
