#!/usr/bin/env python3
"""Verify GPIO22 (homing sensor) reads correctly using original project logic."""
import time
import RPi.GPIO as GPIO

HOMING_SENSOR_PIN = 22

# 完全复刻你主程序里的初始化逻辑
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(HOMING_SENSOR_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

print("Reading GPIO22 (BCM) every 0.5s for 20 seconds.")
print("Manually trigger the sensor (block / unblock) and watch the value.")
print("-" * 50)

try:
    for i in range(40):
        # 原汁原味的读取方式
        val = GPIO.input(HOMING_SENSOR_PIN)
        print(f"[{i:3d}] GPIO22 = {val}")
        time.sleep(0.5)
except KeyboardInterrupt:
    print("\nTest interrupted.")
finally:
    # 按照你主程序的优良习惯，测试完单独清理该引脚
    try:
        GPIO.cleanup(HOMING_SENSOR_PIN)
        print("Cleaned up Pin 22. Done.")
    except Exception:
        pass