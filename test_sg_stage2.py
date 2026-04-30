#!/usr/bin/env python3
"""
Week 5 Stage 2: StallGuard no-load false-trigger test

Run motor 1000 steps with no load. is_stalled() must stay False.
If it triggers, SGTHRS is too sensitive - lower it.
"""
import sys
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module
import time

STEP_PIN = 16
DIR_PIN  = 25
EN_PIN   = 5

SGTHRS    = 100   # StallGuard threshold (lower = less sensitive)
TCOOLTHRS = 300   # CoolStep / StallGuard activation threshold

STEPS = 1000

print("=== Week 5 Stage 2: No-Load False-Trigger Test ===")
print(f"SGTHRS={SGTHRS}, TCOOLTHRS={TCOOLTHRS}, steps={STEPS}")

tmc = tmc2209_module.TMC2209Driver(
    step_pin=STEP_PIN, dir_pin=DIR_PIN, en_pin=EN_PIN,
    uart_dev="/dev/ttyAMA2"
)
tmc.configure(current_ma=600, microsteps=1)
print(f"is_communicating = {tmc.is_communicating()}")
print(f"version          = 0x{tmc.get_version():02X}")

tmc.setup_stallguard(sgthrs=SGTHRS, tcoolthrs=TCOOLTHRS, sw_threshold=3)
tmc.reset_stall()

# Match Week 4 speed parameters - default values too aggressive
tmc.max_speed   = 500
tmc.accel       = 200
tmc.start_speed = 50

tmc.set_enabled(True)
time.sleep(0.1)

print("\n>>> Running motor (no load expected)...")
tmc.step_pulse(STEPS, True)

stalled = tmc.is_stalled()
sg_result = tmc.get_sg_result()

tmc.set_enabled(False)

print(f"\n=== Result ===")
print(f"is_stalled()  = {stalled}")
print(f"SG_RESULT     = {sg_result}")

if stalled:
    print("FAIL: false stall trigger - SGTHRS too high (too sensitive)")
    print("      Try lowering SGTHRS (e.g. 80, 60, 40)")
else:
    print("PASS: no false trigger at SGTHRS=" + str(SGTHRS))
