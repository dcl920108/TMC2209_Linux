#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
test_do_homing_once.py
Standalone test of the C++ driver version of do_homing_once().
Single homing cycle, no Kivy UI.
"""
import sys
import time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')

from tmc2209_module import TMC2209Driver
import RPi.GPIO as GPIO

# ---- Motor pins ----
STEP_PIN = 16
DIR_PIN  = 25
EN_PIN   = 5

# ---- Homing constants (mirrors main program) ----
HOMING_SENSOR_PIN      = 22
HOMING_MAX_BACK_STEPS  = 2000
HOMING_BACKOFF_DELAY_S = 0.5
HOMING_BACKOFF_STEPS   = 50
HOMING_ESCAPE_STEPS    = 500
HOMING_ESCAPE_DELAY_S  = 0.10

# ---- Global TMC instance (program lifecycle scope) ----
tmc = None

def do_homing_once() -> bool:
    """One homing cycle. Returns True on success, False otherwise."""
    global tmc
    success = False
    try:
        # 1) Switch to run profile
        tmc.set_current_profile(run_ma=600, hold_multiplier=0.9)
        tmc.max_speed   = 300
        tmc.accel       = 100
        tmc.start_speed = 50
        tmc.set_enabled(True)

        # 2) Escape if sensor already HIGH
        if GPIO.input(HOMING_SENSOR_PIN) == 1:
            print(f"[HOMING] Sensor HIGH at start escape +{HOMING_ESCAPE_STEPS}")
            tmc.step_pulse(HOMING_ESCAPE_STEPS, True)
            time.sleep(HOMING_ESCAPE_DELAY_S)
            if GPIO.input(HOMING_SENSOR_PIN) == 1:
                print("[HOMING][WARN] Still HIGH after escape")

        # 3) Negative single-step search (pre/post check)
        def backoff(tag, steps_done):
            print(f"[HOMING] Trigger {tag} @ {steps_done}")
            time.sleep(HOMING_BACKOFF_DELAY_S)
            tmc.step_pulse(HOMING_BACKOFF_STEPS, True)

        steps_done = 0
        while steps_done < HOMING_MAX_BACK_STEPS:
            if GPIO.input(HOMING_SENSOR_PIN) == 1:
                backoff("BEFORE", steps_done); success = True; break
            tmc.step_pulse(1, False)
            steps_done += 1
            if GPIO.input(HOMING_SENSOR_PIN) == 1:
                backoff("AFTER", steps_done); success = True; break
            time.sleep(0.002)

        if not success:
            print(f"[HOMING][WARN] Reach limit {HOMING_MAX_BACK_STEPS} but sensor still LOW")

        return success

    except Exception as e:
        print(f"[HOMING][ERROR] {e}")
        return False

    finally:
        # 4) Drop to standby + disable
        try:
            tmc.set_current_profile(run_ma=300, hold_multiplier=0.05)
            tmc.set_enabled(False)
        except Exception:
            pass

# ---- Main ----
if __name__ == "__main__":
    print("=" * 50)
    print("test_do_homing_once - single cycle")
    print("=" * 50)

    # One-time init (program startup)
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(HOMING_SENSOR_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

    tmc = TMC2209Driver(STEP_PIN, DIR_PIN, EN_PIN)
    tmc.configure(current_ma=600, microsteps=1)

    # Run one homing cycle
    result = do_homing_once()
    print(f"\nResult: {'SUCCESS' if result else 'FAIL'}")

    # Cleanup (program exit)
    GPIO.cleanup(HOMING_SENSOR_PIN)
    del tmc
    print("Done.")
