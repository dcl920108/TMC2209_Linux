#!/usr/bin/env python3
"""Verify DIAG pin behavior during manual stall.
Chunk-based motion with SG_RESULT + DIAG reading between chunks."""
import sys, time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

tmc = tmc2209_module.TMC2209Driver(16, 25, 5)
tmc.configure(current_ma=600, microsteps=1)
tmc.max_speed   = 1000
tmc.accel       = 500
tmc.start_speed = 100
tmc.setup_stallguard(sgthrs=70, tcoolthrs=0xFFFFF)
tmc.set_enabled(True)

print("Small chunks of 200 steps, reading SG + DIAG between each.")
print("PINCH THE MOTOR SHAFT during the middle chunks.")
for i in range(3, 0, -1):
    print(f"Starting in {i}...", flush=True); time.sleep(1)
print("GO!", flush=True)

for chunk in range(100):
    tmc.step_pulse(200, True)
    sg = tmc.get_sg_result()
    diag = tmc.read_diag_level()
    mark = " <-- PINCH?" if sg < 100 else ""
    print(f"chunk {chunk:3d}: SG={sg:4d}  DIAG={diag}{mark}", flush=True)
    if diag == 1:                                       # 新增
        print(f">>> STALL DETECTED at chunk {chunk}, stopping!", flush=True)
        break    

tmc.set_enabled(False)
print("Done.")
