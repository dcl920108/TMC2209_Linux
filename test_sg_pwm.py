#!/usr/bin/env python3
"""Test pwm_autoscale + pwm_autograd effect on SG_RESULT under load."""
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
tmc.max_speed   = 500
tmc.accel       = 150
tmc.start_speed = 100
tmc.setup_stallguard(sgthrs=35, tcoolthrs=0xFFFFF, sw_threshold=3)
tmc.set_enabled(True)
tmc.reset_stall()

print("=" * 50, flush=True)
print("SG_RESULT monitor — pinch shaft during GO!", flush=True)
print("=" * 50, flush=True)

for i in range(3, 0, -1):
    print(f"Starting in {i}...", flush=True)
    time.sleep(1)

print("GO! PINCH NOW!", flush=True)

def monitor():
    for _ in range(40):
        sg = tmc.get_sg_result()
        print(f"SG={sg:4d}  threshold={35*2}", flush=True)
        time.sleep(0.25)

t = threading.Thread(target=monitor, daemon=True)
t.start()
tmc.step_pulse(10000, True)
t.join()

tmc.set_enabled(False)
print("Done.", flush=True)
