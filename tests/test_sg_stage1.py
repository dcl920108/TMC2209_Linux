#!/usr/bin/env python3
"""Week 5 StallGuard - Stage 1 only: API calls, motor does not move."""
import sys
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

STEP_PIN = 16
DIR_PIN  = 25
EN_PIN   = 5

print("Creating driver...")
tmc = tmc2209_module.TMC2209Driver(STEP_PIN, DIR_PIN, EN_PIN)
print("  OK")

print("Configure (800 mA, 16 microsteps)...")
tmc.configure(800, 16)
print("  OK")

print("setup_stallguard(100, 400, 3)...")
tmc.setup_stallguard(100, 400, 3)
print("  OK")

print(f"is_stalled() initial = {tmc.is_stalled()}")
print("reset_stall()...")
tmc.reset_stall()
print(f"is_stalled() after reset = {tmc.is_stalled()}")

print("\nStage 1 PASS - all API calls work, no segfault.")
