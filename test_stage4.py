#!/usr/bin/env python3
"""Stage 4: step_until_stall() - run motor up to N steps, 
   stop on stall (DIAG), return actual step count."""

import sys, time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module


def step_until_stall(tmc, max_steps, direction, chunk_size=200):
    """Step motor up to max_steps, break on DIAG rising.
    Returns (actual_steps, stalled)."""
    total_steps = 0
    while total_steps < max_steps:
        remaining = max_steps - total_steps
        this_chunk = min(chunk_size, remaining)
        tmc.step_pulse(this_chunk, direction)
        total_steps += this_chunk
        
        if tmc.read_diag_level() == 1:
            return (total_steps, True)
    
    return (total_steps, False)


# Main test
tmc = tmc2209_module.TMC2209Driver(16, 25, 5)
tmc.configure(current_ma=600, microsteps=1)
tmc.max_speed   = 1000
tmc.accel       = 500
tmc.start_speed = 100
tmc.setup_stallguard(sgthrs=70, tcoolthrs=0xFFFFF)
tmc.set_enabled(True)

MAX_STEPS = 3000

print("=" * 50)
print(f"Stage 4: step_until_stall()")
print(f"Will attempt {MAX_STEPS} steps (~3 sec). Pinch shaft any time.")
print("=" * 50)

for i in range(3, 0, -1):
    print(f"Starting in {i}...", flush=True); time.sleep(1)
print("GO!", flush=True)

t0 = time.time()
actual, stalled = step_until_stall(tmc, max_steps=MAX_STEPS, direction=True, chunk_size=200)
elapsed = time.time() - t0

print()
print("=" * 50)
if stalled:
    print(f"STALLED at step {actual}/{MAX_STEPS}")
    print(f"Elapsed: {elapsed:.2f}s")
else:
    print(f"Completed all {actual} steps (no stall)")
    print(f"Elapsed: {elapsed:.2f}s")
print("=" * 50)

tmc.set_enabled(False)
print("Done.")
