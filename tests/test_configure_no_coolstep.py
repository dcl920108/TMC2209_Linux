#!/usr/bin/env python3
import sys, time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
from tmc2209_module import TMC2209Driver
import tmc2209_module

tmc = TMC2209Driver(step_pin=16, dir_pin=25, en_pin=5)

# Manually do what configure() does, but skip enableCoolStep
# We need to call individual stepper_ methods, but they're not exposed to Python.
# So instead just don't call configure at all, and try to transit settings via UART.

# Skip configure entirely - use defaults
print(f"version=0x{tmc.get_version():02X}")
print(f"is_communicating = {tmc.is_communicating()}")

tmc.max_speed = 500
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
