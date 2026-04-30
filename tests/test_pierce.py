#!/usr/bin/env python3
"""Pierce membrane test - count DIAG triggers per step, stop at threshold."""
import sys, time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

# Tune this based on real hardware behavior. Start small (5), increase if 
# membrane pierce alone trips it; decrease if reaching tube bottom never trips.
THRESHOLD = 20
MAX_STEPS = 12000

tmc = tmc2209_module.TMC2209Driver(16, 25, 5)
tmc.configure(current_ma=600, microsteps=1)
tmc.max_speed   = 500
tmc.accel       = 150
tmc.start_speed = 100
tmc.setup_stallguard(sgthrs=70, tcoolthrs=0xFFFFF)
tmc.set_enabled(True)

print(f"Pierce test: max_steps={MAX_STEPS}, threshold={THRESHOLD}")
print("Press the shaft against membrane / tube...")
for i in range(3, 0, -1):
    print(f"Starting in {i}...", flush=True); time.sleep(1)
print("GO!", flush=True)

t0 = time.time()
actual = tmc.step_pulse_until_triggered(MAX_STEPS, True, THRESHOLD)
elapsed = time.time() - t0

print()
if actual < MAX_STEPS:
    print(f"BOTTOM REACHED at step {actual} / {MAX_STEPS}")
else:
    print(f"Completed all {actual} steps without hitting threshold")
print(f"Elapsed: {elapsed:.2f}s")

tmc.set_enabled(False)
print("Done.")
