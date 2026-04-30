#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GPIO23 reset verification with KivyMD button.

Mirror of the legacy PyTmcStepper test program, but uses the new C++
TMC2209Driver underneath. The legacy program crashes on the 2nd button
press due to RPi.GPIO add_event_detect() residue on GPIO23.

This version uses pigpio polling (no ISR), so the destructor leaves
GPIO23 clean. Click the button as many times as you want — should never
crash.

Test method: click the button 5+ times in a row.
Pass: every click runs and completes.
Fail: any click after the 1st throws an exception.
"""

import sys
import time

sys.path.insert(0, '/home/nero/TMC2209_Linux/build')
import tmc2209_module

from kivy.metrics import dp
from kivymd.app import MDApp
from kivymd.uix.screen import MDScreen
from kivymd.uix.boxlayout import MDBoxLayout
from kivymd.uix.button import MDButton, MDButtonIcon
from kivymd.uix.label import MDLabel

# Test parameters
MAX_STEPS        = 12000
BOTTOM_THRESHOLD = 3       # match legacy test program for direct comparison
WARMUP_STEPS     = 200


class GpioResetTestApp(MDApp):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.click_count = 0

    def build(self):
        self.theme_cls.theme_style = "Light"
        self.theme_cls.primary_palette = "Green"

        screen = MDScreen()

        layout = MDBoxLayout(
            orientation="vertical",
            spacing=dp(20),
            padding=dp(20),
            pos_hint={"center_x": 0.5, "center_y": 0.5},
            size_hint=(0.8, 0.8),
        )

        # Click counter label
        self.counter_label = MDLabel(
            text="Clicks: 0",
            halign="center",
            font_style="Display",
            role="small",
            size_hint_y=None,
            height=dp(60),
        )
        layout.add_widget(self.counter_label)

        # Status label
        self.status_label = MDLabel(
            text="Ready — press the button",
            halign="center",
            font_style="Title",
            role="medium",
            size_hint_y=None,
            height=dp(50),
        )
        layout.add_widget(self.status_label)

        # Big run button (same look as legacy)
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
        layout.add_widget(run_button)

        screen.add_widget(layout)
        return screen

    def motor_control(self, *args):
        """One full create-use-destroy cycle of the C++ driver."""
        self.click_count += 1
        self.counter_label.text = f"Clicks: {self.click_count}"
        self.status_label.text = f"Click #{self.click_count} — running..."

        print(f"\n{'='*55}")
        print(f"CLICK #{self.click_count}")
        print(f"{'='*55}")

        # ===== Construct =====
        print(f"[MC] constructing TMC2209Driver...")
        tmc = tmc2209_module.TMC2209Driver(16, 25, 5)

        # ===== Configure =====
        print(f"[MC] configuring (current=600mA, microsteps=1, sgthrs=70)...")
        tmc.configure(current_ma=600, microsteps=1)
        tmc.max_speed   = 500
        tmc.accel       = 150
        tmc.start_speed = 100

        ver  = tmc.get_version()
        comm = tmc.is_communicating()
        print(f"[MC] version=0x{ver:02X}  communicating={comm}")

        # ===== Run =====
        tmc.set_enabled(True)
        print(f"[MC] running {MAX_STEPS} steps "
              f"(threshold={BOTTOM_THRESHOLD}, warmup={WARMUP_STEPS})...")

        t0 = time.time()
        actual = tmc.step_pulse_until_triggered(
            MAX_STEPS, True, BOTTOM_THRESHOLD, WARMUP_STEPS
        )
        elapsed = time.time() - t0

        if actual < MAX_STEPS:
            print(f"[MC] STOPPED at step {actual} / {MAX_STEPS}  ({elapsed:.2f}s)")
            result_text = f"Click #{self.click_count} — stopped at step {actual}"
        else:
            print(f"[MC] completed all {actual} steps  ({elapsed:.2f}s)")
            result_text = f"Click #{self.click_count} — completed {actual} steps"

        # ===== Disable =====
        tmc.set_enabled(False)

        # ===== Destruct =====
        # Critical: del tmc triggers C++ destructor.
        # If destructor cleanup is broken, the NEXT click will crash.
        print(f"[MC] destructing...")
        del tmc
        print(f"[MC] DONE — ready for next click")

        self.status_label.text = result_text + " — ready for next click"


if __name__ == "__main__":
    GpioResetTestApp().run()
