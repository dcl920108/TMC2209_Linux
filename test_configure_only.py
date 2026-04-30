#!/usr/bin/env python3
"""
Minimal configure() test.
Only configures the chip, reads IFCNT, no motor motion.
Confirms setRMSCurrent path is safe.
"""
import sys
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module
import time

print("=== Configure-only test (no motor motion) ===")

tmc = tmc2209_module.TMC2209Driver(step_pin=16, dir_pin=25, en_pin=5)

print("Calling configure(600 mA, 16 microsteps)...")
tmc.configure(current_ma=600, microsteps=16)
print("configure() returned")

print(f"is_communicating = {tmc.is_communicating()}")
print(f"version          = 0x{tmc.get_version():02X}")
print(f"ifcnt            = {tmc.get_ifcnt()}")

print("\nNot enabling motor. Done.")
