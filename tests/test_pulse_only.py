import sys
import time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')

from tmc2209_module import TMC2209Driver

tmc = TMC2209Driver(step_pin=16, dir_pin=25, en_pin=5)
tmc.max_speed = 500
tmc.accel = 200
tmc.start_speed = 50
tmc.set_enabled(True)

# print("Forward 1600 steps...")
# tmc.step_pulse(1600, True) #2000 4/27/2026

# time.sleep(2)

print("Reverse 2000 steps...")
tmc.step_pulse(1000, False)

tmc.set_enabled(False)
del tmc
print("Done.")
