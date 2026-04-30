#!/usr/bin/env python3
"""UART communication test - read IFCNT register."""
import sys
import time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

tmc = tmc2209_module.TMC2209Driver(16, 25, 5)
tmc.configure(800, 16)

print(f"is_communicating() = {tmc.is_communicating()}")

print("\nRead IFCNT 3 times (should be stable, same value):")
for i in range(3):
    n = tmc.get_ifcnt()
    print(f"  [{i}] IFCNT = {n}")
    time.sleep(0.1)

print("\nCall configure() again (triggers multiple UART writes)...")
tmc.configure(800, 16)

print("\nRead IFCNT 3 more times (should be HIGHER than before):")
for i in range(3):
    n = tmc.get_ifcnt()
    print(f"  [{i}] IFCNT = {n}")
    time.sleep(0.1)
