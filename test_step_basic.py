# #!/usr/bin/env python3
# import sys, time
# sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
# import tmc2209_module

# tmc = tmc2209_module.TMC2209Driver(16, 25, 5)
# tmc.configure(current_ma=600, microsteps=1)
# tmc.max_speed = 1000
# tmc.accel = 500
# tmc.start_speed = 100
# tmc.set_enabled(True)

# print("正向 100 步 (dir=True)")
# tmc.step_pulse(500, True)
# time.sleep(1)

# # print("反向 100 步 (dir=False)")
# # tmc.step_pulse(100, False)
# # time.sleep(1)

# # print("反向单步 50 次 (回零模式)")
# # for i in range(50):
# #     tmc.step_pulse(1, False)
# #     time.sleep(0.002)

# tmc.set_enabled(False)
# print("完成")

import sys
import time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')

from tmc2209_module import TMC2209Driver

tmc = TMC2209Driver(step_pin=16, dir_pin=25, en_pin=5)
tmc.max_speed = 500
tmc.accel = 200
tmc.start_speed = 50
tmc.set_enabled(True)

print("反向单步 500 次 (回零模式)")
for i in range(500):
    tmc.step_pulse(1, False)
    time.sleep(0.002)

tmc.set_enabled(False)
del tmc
print("Done.")
