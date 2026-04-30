#!/usr/bin/env python3
import sys, time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
from tmc2209_module import TMC2209Driver

tmc = TMC2209Driver(step_pin=16, dir_pin=25, en_pin=5)
tmc.configure(current_ma=600, microsteps=1) # 16------4/15/2026
print(f"version=0x{tmc.get_version():02X}")

tmc.max_speed = 1000 #500------4/15/2026
tmc.accel = 200
tmc.start_speed = 50
tmc.set_enabled(True)

print("Forward 2000 steps...")
tmc.step_pulse(2000, True)
time.sleep(2)
print("Reverse 2000 steps...")
tmc.step_pulse(2000, False)

tmc.set_enabled(False)
del tmc
print("Done.")
