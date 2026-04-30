#!/usr/bin/env python3
"""
Week 5 StallGuard tuning - live SG_RESULT readout.
Motor runs continuously. Press Ctrl+C to stop.

Workflow:
  1. Watch no-load baseline (stable high value)
  2. Gently load the shaft with fingers, watch value drop
  3. Record min/max, pick SGTHRS = (min_loaded / 2) roughly
"""
import sys
import time
import threading
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

STEP_PIN = 16
DIR_PIN  = 25
EN_PIN   = 5

SGTHRS    = 0      # 0 during tuning - we only read, not trigger
TCOOLTHRS = 400

tmc = tmc2209_module.TMC2209Driver(STEP_PIN, DIR_PIN, EN_PIN)
tmc.configure(800, 16)
tmc.setup_stallguard(SGTHRS, TCOOLTHRS, 3)
tmc.set_enabled(True)

# Run motor in background thread so main thread can poll
stop_flag = {'stop': False}

def motor_loop():
    while not stop_flag['stop']:
        tmc.step_pulse(500, True)

t = threading.Thread(target=motor_loop, daemon=True)
t.start()

print("Polling SG_RESULT (Ctrl+C to stop)")
print("----------------------------------")
try:
    while True:
        sg = tmc.get_sg_result()
        print(f"  SG_RESULT = {sg:4d}   (trigger if < {2*SGTHRS})")
        time.sleep(0.1)
except KeyboardInterrupt:
    print("\nStopping...")
    stop_flag['stop'] = True
    time.sleep(0.5)
    tmc.set_enabled(False)
    print("Motor disabled.")
