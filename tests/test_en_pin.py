#!/usr/bin/env python3
"""Test EN pin - motor should lock when enabled, free when disabled."""
import sys
import time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

tmc = tmc2209_module.TMC2209Driver(16, 25, 5)
tmc.configure(800, 16)

print("Motor DISABLED - try turning shaft, should spin freely")
tmc.set_enabled(False)
input("  [Press Enter after checking]")

print("\nMotor ENABLED - try turning shaft, should be locked")
tmc.set_enabled(True)
input("  [Press Enter after checking]")

print("\nDisabling motor.")
tmc.set_enabled(False)
