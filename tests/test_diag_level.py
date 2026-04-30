#!/usr/bin/env python3
"""Week 5 - Direct GPIO23 level monitoring via C++ driver.
Polls DIAG pin every 20ms during motor run.
Detects whether DIAG ever goes HIGH during pinch.
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
tmc.max_speed   = 500
tmc.accel       = 200
tmc.start_speed = 50
tmc.setup_stallguard(sgthrs=90, tcoolthrs=400, sw_threshold=3)
tmc.set_enabled(True)
tmc.reset_stall()

stop_flag = False
high_count = 0
sample_count = 0
first_high_time = None

def watcher():
    global high_count, sample_count, first_high_time
    start = time.time()
    while not stop_flag:
        level = tmc.read_diag_level()
        sample_count += 1
        if level == 1:
            high_count += 1
            if first_high_time is None:
                first_high_time = time.time() - start
            print(f"[{time.time()-start:5.2f}s] DIAG=HIGH  (hits={high_count})", flush=True)
        time.sleep(0.02)

t = threading.Thread(target=watcher, daemon=True)
t.start()

print("=" * 50, flush=True)
print("Direct DIAG pin monitoring (bypassing ISR)", flush=True)
print("Motor will run 10000 steps (~10s).", flush=True)
print("=" * 50, flush=True)

for i in range(3, 0, -1):
    print(f"Starting in {i}...", flush=True)
    time.sleep(1)

print("GO! PINCH NOW!", flush=True)
tmc.step_pulse(10000, True)

stop_flag = True
time.sleep(0.1)

print(f"\n===== RESULT =====", flush=True)
print(f"Total samples  : {sample_count}", flush=True)
print(f"DIAG HIGH hits : {high_count}", flush=True)
if first_high_time is not None:
    print(f"First HIGH at  : {first_high_time:.2f}s", flush=True)
print(f"Final is_stalled() from C++ ISR = {tmc.is_stalled()}", flush=True)

if high_count == 0:
    print("--> DIAG never went HIGH. TMC2209 not outputting stall signal.", flush=True)
elif high_count > 0 and not tmc.is_stalled():
    print("--> DIAG fired but ISR not catching it. pigpio ISR setup issue.", flush=True)
else:
    print("--> Both DIAG and ISR working.", flush=True)

tmc.set_enabled(False)
