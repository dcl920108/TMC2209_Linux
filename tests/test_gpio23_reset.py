#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GPIO23 reset verification test.

Goal: verify that C++ TMC2209Driver destructor cleans up GPIO23 properly,
so a Python process can create-use-destroy the driver multiple times in
sequence without the PyTmcStepper-style residual-ISR bug.

Each round: construct -> configure -> run pierce -> destruct.
Pass criterion: 5 consecutive rounds all complete without error.
"""

import sys
import time
import gc

sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

NUM_ROUNDS    = 5
MAX_STEPS     = 4000      # short run, just enough to verify a full cycle
THRESHOLD     = 20
WARMUP_STEPS  = 200

# Working parameters (Week 7 locked, Week 8 verified)
CURRENT_MA    = 600
MICROSTEPS    = 1
SGTHRS        = 70
TCOOLTHRS     = 0xFFFFF
MAX_SPEED     = 500
ACCEL         = 150
START_SPEED   = 100


def run_one_round(round_idx: int) -> bool:
    print(f"\n{'='*55}")
    print(f"ROUND {round_idx}/{NUM_ROUNDS}")
    print(f"{'='*55}")

    try:
        # Construct
        print(f"[R{round_idx}] constructing TMC2209Driver...")
        tmc = tmc2209_module.TMC2209Driver(16, 25, 5)

        # Configure (sgthrs + tcoolthrs use defaults from .h)
        print(f"[R{round_idx}] configuring...")
        tmc.configure(current_ma=CURRENT_MA, microsteps=MICROSTEPS)

        # Set motion profile
        tmc.max_speed   = MAX_SPEED
        tmc.accel       = ACCEL
        tmc.start_speed = START_SPEED

        # Verify communication
        ver = tmc.get_version()
        comm = tmc.is_communicating()
        sc = tmc.is_stealthchop_enabled()
        print(f"[R{round_idx}] version=0x{ver:02X}  communicating={comm}  stealthchop={sc}")

        if ver != 0x21:
            print(f"[R{round_idx}] FAIL: version != 0x21")
            return False

        # Enable + run
        tmc.set_enabled(True)
        print(f"[R{round_idx}] running {MAX_STEPS} steps (threshold={THRESHOLD}, warmup={WARMUP_STEPS})...")

        t0 = time.time()
        actual = tmc.step_pulse_until_triggered(MAX_STEPS, True, THRESHOLD, WARMUP_STEPS)
        elapsed = time.time() - t0

        if actual < MAX_STEPS:
            print(f"[R{round_idx}] STOPPED at step {actual} / {MAX_STEPS} ({elapsed:.2f}s)")
        else:
            print(f"[R{round_idx}] completed all {actual} steps without trigger ({elapsed:.2f}s)")

        # Disable
        tmc.set_enabled(False)

        # Destruct (explicit del + GC to force destructor call)
        print(f"[R{round_idx}] destructing...")
        del tmc
        gc.collect()
        print(f"[R{round_idx}] DONE")
        return True

    except Exception as e:
        print(f"[R{round_idx}] EXCEPTION: {type(e).__name__}: {e}")
        return False


def main():
    print(f"\nGPIO23 reset test - {NUM_ROUNDS} consecutive rounds\n")
    results = []

    for i in range(1, NUM_ROUNDS + 1):
        ok = run_one_round(i)
        results.append(ok)
        if not ok:
            print(f"\n!!! Round {i} FAILED — stopping early !!!\n")
            break
        if i < NUM_ROUNDS:
            print(f"\n[gap] sleeping 1s before next round...\n")
            time.sleep(1.0)

    # Summary
    print(f"\n{'='*55}")
    print(f"SUMMARY")
    print(f"{'='*55}")
    passed = sum(results)
    total  = len(results)
    print(f"Rounds passed: {passed}/{total}")
    for i, ok in enumerate(results, 1):
        print(f"  Round {i}: {'PASS' if ok else 'FAIL'}")

    if passed == NUM_ROUNDS:
        print(f"\n*** ALL {NUM_ROUNDS} ROUNDS PASSED — GPIO23 cleanup verified ***")
        sys.exit(0)
    else:
        print(f"\n*** {total-passed} round(s) FAILED — destructor has issue ***")
        sys.exit(1)


if __name__ == "__main__":
    main()
