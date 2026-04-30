#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AGD motor full cycle demo (KivyMD): pierce-to-bottom -> homing -> done."""

import sys
import time
sys.path.insert(0, '/home/nero/TMC2209_Linux/build')

from kivy.metrics import dp
from kivymd.app import MDApp
from kivymd.uix.screen import MDScreen
from kivymd.uix.button import MDButton, MDButtonIcon

from tmc2209_module import TMC2209Driver
import RPi.GPIO as GPIO

# ==================== Constants ====================
STEP_PIN = 16
DIR_PIN  = 25
EN_PIN   = 5

SGTHRS         = 70
TCOOLTHRS      = 0xFFFFF
THRESHOLD      = 20
WARMUP_STEPS   = 200
MAX_STEPS      = 12000

HOMING_SENSOR_PIN     = 22
HOMING_MAX_BACK_STEPS = 2000
HOMING_BACKOFF_STEPS  = 50


class MotorCycleApp(MDApp):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.tmc = None

    def build(self):
        self.theme_cls.theme_style = "Light"
        self.theme_cls.primary_palette = "Green"

        screen = MDScreen()

        run_button = MDButton(
            MDButtonIcon(
                icon="motorbike-electric",
                theme_font_size="Custom",
                theme_icon_color="Custom",
                icon_color=(1, 1, 1, 1),
                font_size="128sp",
                pos_hint={"center_x": 0.5, "center_y": 0.5},
            ),
            style="elevated",
            pos_hint={"center_x": 0.5, "center_y": 0.5},
            theme_width="Custom",
            height="80dp",
            theme_bg_color="Custom",
            md_bg_color=(0.2, 0.7, 0.3, 1),
            size_hint=(None, None),
            size=(dp(240), dp(240)),
            on_release=self.motor_control,
        )
        screen.add_widget(run_button)

        return screen

    def motor_control(self, *args):
        """Pierce to bottom -> Homing -> Done."""

        # ---- Cleanup old TMC if exists ----
        if self.tmc is not None:
            try:
                print("[MC] Cleaning up old TMC object...")
                self.tmc.set_enabled(False)
                del self.tmc
                self.tmc = None
                time.sleep(0.5)
            except Exception as e:
                print(f"[MC] Cleanup failed: {e}")
                self.tmc = None

        # ---- GPIO22 sensor init ----
        GPIO.setwarnings(False)
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(HOMING_SENSOR_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

        # ---- Motor init ----
        tmc = TMC2209Driver(step_pin=STEP_PIN, dir_pin=DIR_PIN, en_pin=EN_PIN)
        tmc.configure(current_ma=600, microsteps=1, sgthrs=SGTHRS, tcoolthrs=TCOOLTHRS)
        tmc.max_speed   = 500
        tmc.accel       = 150
        tmc.start_speed = 100
        tmc.set_enabled(True)

        print("=" * 50)
        print("[MC] AGD Motor Demo: pierce -> homing -> done")
        print("=" * 50)

        pierce_steps = 0
        homing_steps = 0
        pierce_success = False
        homing_success = False

        try:
            # ==================== Stage 1: Pierce to bottom ====================
            print("\n[MC] Stage 1: Pierce to bottom...")
            t0 = time.time()
            pierce_steps = tmc.step_pulse_until_triggered(
                max_steps=MAX_STEPS,
                dir=True,
                threshold=THRESHOLD,
                warmup_steps=WARMUP_STEPS,
            )
            pierce_elapsed = time.time() - t0

            if pierce_steps < MAX_STEPS:
                print(f"[MC] ✓ 扎到底！(步数: {pierce_steps}, 耗时: {pierce_elapsed:.2f}s)")
                pierce_success = True
            else:
                print(f"[MC] ✗ 扎到底失败：达到最大步数 {MAX_STEPS}")

            time.sleep(0.5)

            # ==================== Stage 2: Homing ====================
            print("\n[MC] Stage 2: Homing to GPIO22...")
            steps_done = 0
            while steps_done < HOMING_MAX_BACK_STEPS:
                if GPIO.input(HOMING_SENSOR_PIN) == 1:
                    print(f"[MC] ✓ 找到零点！(步数: {steps_done})")
                    time.sleep(0.5)
                    print(f"[MC]   退 {HOMING_BACKOFF_STEPS} 步...")
                    tmc.step_pulse(HOMING_BACKOFF_STEPS, True)
                    homing_steps = steps_done
                    homing_success = True
                    break
                tmc.step_pulse(1, False)
                steps_done += 1
                time.sleep(0.002)

            if not homing_success:
                print(f"[MC] ✗ 回零失败：达到最大步数 {HOMING_MAX_BACK_STEPS}")

        finally:
            try:
                tmc.set_enabled(False)
            except Exception:
                pass

            self.tmc = tmc

            try:
                GPIO.cleanup(HOMING_SENSOR_PIN)
            except Exception:
                pass

            time.sleep(0.5)

            print("\n" + "=" * 50)
            print("[MC] Cycle Summary")
            print("=" * 50)
            print(f"[MC] Pierce:  {'✓' if pierce_success else '✗'}  steps={pierce_steps}")
            print(f"[MC] Homing:  {'✓' if homing_success else '✗'}  steps={homing_steps}")
            print("[MC] 电机控制完成！")


if __name__ == "__main__":
    MotorCycleApp().run()
