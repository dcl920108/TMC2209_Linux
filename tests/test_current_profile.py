#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
test_current_profile.py — PCR-style thermal cycle test.

Cycle x2:
  Phase 1: slow rotate 2 min (run=600mA, hold=0.9)
  Phase 2: idle      1 min   (run=300mA, hold=0.05)
"""
import sys
import time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')

from tmc2209_module import TMC2209Driver

STEP_PIN = 16
DIR_PIN  = 25
EN_PIN   = 5

# Slow rotation parameters (Week 9 plan A)
SLOW_MAX_SPEED   = 200
SLOW_ACCEL       = 100
SLOW_START_SPEED = 50

# Phase durations (seconds)
ROTATE_SECONDS = 120   # 2 min
IDLE_SECONDS   = 60    # 1 min

def rotate_for(tmc, seconds, label):
    """Continuously step for given seconds with 10s heartbeat prints."""
    chunk_steps = SLOW_MAX_SPEED * 10   # 10s worth of steps per call
    total_chunks = seconds // 10
    for i in range(total_chunks):
        tmc.step_pulse(chunk_steps, True)
        print(f"    [{label}] {(i+1)*10}s / {seconds}s")

def idle_for(seconds, label):
    """Idle with 10s heartbeat prints."""
    elapsed = 0
    while elapsed < seconds:
        chunk = min(10, seconds - elapsed)
        time.sleep(chunk)
        elapsed += chunk
        print(f"    [{label}] {elapsed}s / {seconds}s")

print("=" * 60)
print("setCurrentProfile — PCR-style thermal cycle test (x2)")
print("=" * 60)

tmc = TMC2209Driver(STEP_PIN, DIR_PIN, EN_PIN)
tmc.configure(current_ma=600, microsteps=1)
tmc.max_speed   = SLOW_MAX_SPEED
tmc.accel       = SLOW_ACCEL
tmc.start_speed = SLOW_START_SPEED
print(f"[INIT] configure + speed (max={SLOW_MAX_SPEED}, accel={SLOW_ACCEL})\n")

for cycle in range(1, 3):
    print(f"===== CYCLE {cycle} =====")

    # --- Rotate 2 min ---
    print(f"[{cycle}.1] set_current_profile(600mA, hold=0.9) → ROTATE 2 min")
    tmc.set_current_profile(run_ma=600, hold_multiplier=0.9)
    tmc.set_enabled(True)
    rotate_for(tmc, ROTATE_SECONDS, f"cycle{cycle}-rotate")

    # --- Idle 1 min ---
    print(f"\n[{cycle}.2] set_current_profile(300mA, hold=0.05) → IDLE 1 min")
    tmc.set_current_profile(run_ma=300, hold_multiplier=0.05)
    idle_for(IDLE_SECONDS, f"cycle{cycle}-idle")
    print()

tmc.set_enabled(False)
print("=" * 60)
print("PASS — 6 minutes elapsed, 2 cycles completed.")
print("=" * 60)
