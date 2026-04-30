#!/usr/bin/env python3
"""Week 5 StallGuard - Stage 3: manual stall trigger test."""
import sys
import time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

STEP_PIN = 16
DIR_PIN  = 25
EN_PIN   = 5

tmc = tmc2209_module.TMC2209Driver(STEP_PIN, DIR_PIN, EN_PIN)
tmc.configure(current_ma=600, microsteps=1)
tmc.max_speed   = 1000
tmc.accel       = 500
tmc.start_speed = 100

tmc.setup_stallguard(sgthrs=35, tcoolthrs=0xFFFFF, sw_threshold=3)
tmc.set_enabled(True)
tmc.reset_stall()

print("=" * 50, flush=True)
print("Stage 3: Manual stall test (SGTHRS=90)", flush=True)
print("Motor will run 10000 steps (~10s).", flush=True)
print("=" * 50, flush=True)

for i in range(3, 0, -1):
    print(f"Starting in {i}...", flush=True)
    time.sleep(1)

print("GO! PINCH NOW!", flush=True)

tmc.step_pulse(10000, True)

stalled = tmc.is_stalled()
print(f"\nResult: is_stalled() = {stalled}", flush=True)

if stalled:
    print("PASS - stall detected, DIAG/ISR working", flush=True)
else:
    print("NO STALL - DIAG path or ISR likely broken", flush=True)

tmc.set_enabled(False)
