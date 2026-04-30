#!/usr/bin/env python3
"""Week 5 - Live SG_RESULT polling for SGTHRS tuning.
Motor runs continuously. Print SG_RESULT every 0.1s.
Pinch shaft mid-run to see value drop.
"""
import sys
import time
import threading
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

# SGTHRS=0 -> DIAG will not fire; we only want to READ SG_RESULT freely
tmc.setup_stallguard(sgthrs=0, tcoolthrs=400, sw_threshold=3)
tmc.set_enabled(True)

stop_flag = False

def reader():
    while not stop_flag:
        sg = tmc.get_sg_result()
        print(f"SG_RESULT = {sg:4d}   (suggested SGTHRS ~ {sg//2})")
        time.sleep(0.1)

t = threading.Thread(target=reader, daemon=True)
t.start()

print("=" * 50)
print("Motor runs 20000 steps (~20s).")
print("Watch SG_RESULT. Pinch shaft mid-run to see it drop.")
print("Ctrl+C to stop early.")
print("=" * 50)
time.sleep(1)

try:
    tmc.step_pulse(20000, True)
except KeyboardInterrupt:
    pass

stop_flag = True
time.sleep(0.2)
tmc.set_enabled(False)
print("\nDone.")
