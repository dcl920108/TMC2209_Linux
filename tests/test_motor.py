import sys
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')

from tmc2209_module import TMC2209Driver

tmc = TMC2209Driver(step_pin=21, dir_pin=20, en_pin=23)
tmc.configure(current_ma=800, microsteps=8)
tmc.max_speed = 2000
tmc.accel = 500
tmc.start_speed = 100
tmc.set_enabled(True)

print("Moving 200 steps forward...")
tmc.step_pulse(200, True)

print("Done.")
tmc.set_enabled(False)
del tmc
