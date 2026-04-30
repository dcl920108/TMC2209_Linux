import sys
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

tmc = tmc2209_module.TMC2209Driver(16, 25, 5)
tmc.configure(800, 16)

print(">>> Now calling get_ifcnt() <<<")
n = tmc.get_ifcnt()
print(f">>> IFCNT = {n} <<<")
