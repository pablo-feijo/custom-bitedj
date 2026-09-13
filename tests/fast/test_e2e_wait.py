"""Exercise desktop readiness polling without launching a desktop."""

import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch


SPEC = importlib.util.spec_from_file_location(
    "desktop", Path(__file__).resolve().parents[1] / "e2e/test_desktop.py")
desktop = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(desktop)


class DesktopReadiness(unittest.TestCase):
    def setUp(self):
        self.now = 0.0
        clock = patch.object(desktop.time, "monotonic", lambda: self.now)
        sleeper = patch.object(desktop.time, "sleep", self.advance)
        clock.start()
        sleeper.start()
        self.addCleanup(clock.stop)
        self.addCleanup(sleeper.stop)

    def advance(self, seconds):
        self.now += seconds

    def test_default_returns_first_success(self):
        self.assertEqual(desktop.eventually(lambda: "ready"), "ready")
        self.assertEqual(self.now, 0)

    def test_startup_redirect_restarts_stability_window(self):
        def browse():
            if 1.4 <= self.now < 1.8:
                raise AssertionError("Startup redirected to Settings")
            return "Browse"

        self.assertEqual(
            desktop.eventually(browse, timeout=6, stable_for=2), "Browse")
        self.assertGreaterEqual(self.now, 3.8)

    def test_unstable_page_still_times_out(self):
        calls = 0

        def browse():
            nonlocal calls
            calls += 1
            if calls % 2 == 0:
                raise AssertionError("Settings still visible")

        with self.assertRaisesRegex(AssertionError, "Settings still visible"):
            desktop.eventually(browse, timeout=3, stable_for=2)
        self.assertLess(self.now, 3.3)
