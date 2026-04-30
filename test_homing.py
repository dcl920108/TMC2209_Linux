#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Homing test — single-step reverse until GPIO22 HIGH, then back off."""

import sys
import time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')

from tmc2209_module import TMC2209Driver
import RPi.GPIO as GPIO

# ---- Homing constants ----
HOMING_SENSOR_PIN     = 22
HOMING_MAX_BACK_STEPS = 2000
HOMING_BACKOFF_STEPS  = 50

# ---- Sensor init ----
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(HOMING_SENSOR_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

# ---- Motor init ----
tmc = TMC2209Driver(step_pin=16, dir_pin=25, en_pin=5)
tmc.max_speed = 500
tmc.accel = 200
tmc.start_speed = 50
tmc.set_enabled(True)

print("=" * 50)
print("Homing test: single-step reverse until GPIO22=HIGH")
print("=" * 50)

try:
    steps_done = 0
    success = False

    while steps_done < HOMING_MAX_BACK_STEPS:
        if GPIO.input(HOMING_SENSOR_PIN) == 1:
            print(f"✓ 找到零点！(步数: {steps_done})")
            time.sleep(0.5)
            print(f"  退 {HOMING_BACKOFF_STEPS} 步...")
            tmc.step_pulse(HOMING_BACKOFF_STEPS, True)   # forward back-off
            success = True
            break

        tmc.step_pulse(1, False)   # backward 1 step
        steps_done += 1
        time.sleep(0.002)

    if success:
        print("✓ 回零成功！")
    else:
        print(f"✗ 回零失败：达到最大步数 {HOMING_MAX_BACK_STEPS}")

finally:
    tmc.set_enabled(False)
    del tmc
    GPIO.cleanup(HOMING_SENSOR_PIN)
    print("Done.")
