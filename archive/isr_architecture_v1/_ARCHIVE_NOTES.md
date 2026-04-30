# ISR Architecture v1 — Archived 2026-04-30

## What is this?

This is the original implementation of the TMC2209 Linux driver, using
**ISR-based StallGuard** with `gpioSetISRFuncEx` on the DIAG pin.

## Why archived?

During Week 7-8 of the project, the ISR architecture was found to be
unnecessary. The janelia-arduino reference uses **polling** of the
SG_RESULT register, which works more reliably and matches the chip's
official usage pattern.

## What replaced it?

See the current `include/` and `src/` in the parent directory:
- StallGuard configuration moved into `configure()` method
- ISR removed in favor of `step_pulse_until_triggered()` polling loop
- Independent `setupStallGuard()`, `isStalled()`, `resetStall()` methods removed

## Key APIs in this archived version (DO NOT USE)

```cpp
void setupStallGuard(uint8_t sgthrs, uint32_t tcoolthrs, uint32_t sw_threshold);
bool isStalled();
void resetStall();
static void stallISR(int gpio, int level, uint32_t tick, void* userdata);
```

## Reference timeline

- Implemented: Week 5 (April 6-10, 2026)
- Replaced: Week 7-8 (April 20-30, 2026)
- Final working version (Week 8): see parent `include/` and `src/`
