import sys
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

tmc = tmc2209_module.TMC2209Driver(16, 25, 5)
tmc.configure(800, 16)

print(">>> Calling get_version() <<<")
v = tmc.get_version()
print(f">>> Version = 0x{v:02X} (expected 0x21) <<<")
