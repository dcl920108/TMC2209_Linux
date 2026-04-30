#!/usr/bin/env python3
"""Check chopper mode after configure() - is StealthChop or SpreadCycle active?"""
import sys
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

STEP_PIN = 16
DIR_PIN  = 25
EN_PIN   = 5

tmc = tmc2209_module.TMC2209Driver(STEP_PIN, DIR_PIN, EN_PIN)
tmc.configure(current_ma=600, microsteps=1)
tmc.setup_stallguard(sgthrs=90, tcoolthrs=400, sw_threshold=3)
tmc.set_enabled(True)

print("=" * 50, flush=True)
print("Chopper mode check (after configure+setup_stallguard)", flush=True)
print("=" * 50, flush=True)

stealthchop = tmc.is_stealthchop_enabled()
version = tmc.get_version()

print(f"Chip version     : 0x{version:02X}  (expect 0x21)", flush=True)
print(f"StealthChop      : {stealthchop}", flush=True)
print(f"SpreadCycle      : {not stealthchop}", flush=True)
print("", flush=True)

if stealthchop:
    print(">>> StealthChop ACTIVE - en_spreadCycle hypothesis REJECTED", flush=True)
    print(">>> DIAG failure is NOT due to chopper mode", flush=True)
else:
    print(">>> SpreadCycle ACTIVE - en_spreadCycle hypothesis CONFIRMED", flush=True)
    print(">>> This is why DIAG never fires. Fix: force StealthChop.", flush=True)

tmc.set_enabled(False)
