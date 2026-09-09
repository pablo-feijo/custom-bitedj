"""Real desktop + audio smoke tests in a disposable container, with no host ports."""

import array
import math
import sys
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
import uuid
import wave

ROOT = Path(__file__).resolve().parents[2]


def command(*args, timeout=20):
    result = subprocess.run(
        args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout
    )
    if result.returncode:
        raise AssertionError(
            f'{args!r} failed ({result.returncode}): {result.stderr.decode(errors="replace")}'
        )
    return result.stdout


def eventually(check, timeout=30):
    deadline = time.monotonic() + timeout
    last = None
    while time.monotonic() < deadline:
        try:
            return check()
        except (AssertionError, subprocess.SubprocessError) as error:
            last = error
            time.sleep(0.2)
    raise AssertionError(
        f"Condition did not become true in {timeout}s: {last}"
    )


class DesktopE2E(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.container = "bitedj-e2e-" + uuid.uuid4().hex[:12]
        cls.artifacts = ROOT / "test-results" / cls.container
        cls.artifacts.mkdir(parents=True)
        cls.workspace = tempfile.TemporaryDirectory(prefix="bitedj-e2e-")
        cls.addClassCleanup(cls.workspace.cleanup)
        cls.addClassCleanup(cls.stop)
        music = Path(cls.workspace.name)
        # Identical PCM input on every run, long enough for the entire smoke suite.
        samples = array.array(
            "h",
            (
                int(8000 * math.sin(2 * math.pi * 440 * n / 44100))
                for n in range(44100)
            ),
        )
        if sys.byteorder != "little":
            samples.byteswap()
        with wave.open(str(music / "tone.wav"), "wb") as track:
            track.setparams((1, 2, 44100, 0, "NONE", "not compressed"))
            for _ in range(180):
                track.writeframesraw(samples.tobytes())
        command(
            "env",
            f"BITEDJ_TEST_INSTANCE={cls.container}",
            "BITEDJ_TEST_AUTOMATED=1",
            f"BITEDJ_TEST_MUSIC_DIR={music}",
            "bash",
            str(ROOT / "scripts/test/run-gui-test.sh"),
            timeout=90,
        )
        cls.verify_owner()
        eventually(cls.window_ready, timeout=60)
        cls.inside(
            "xdotool",
            "search",
            "--onlyvisible",
            "--name",
            "^Mixxx$",
            "windowactivate",
            "--sync",
        )
        # Window creation precedes skin/control initialization. Establish a
        # known page with an idempotent select action before exercising clicks.
        probe = cls("test_navigation_and_reselect")

        def skin_ready():
            probe.click(950, 20)
            probe.assert_selected_tab(4)

        eventually(skin_ready, timeout=60)

    @classmethod
    def inside(cls, *args, timeout=20):
        return command(
            "docker",
            "exec",
            "-e",
            "DISPLAY=:99",
            cls.container,
            *args,
            timeout=timeout,
        )

    @classmethod
    def window_ready(cls):
        geometry = cls.inside(
            "xdotool",
            "search",
            "--onlyvisible",
            "--name",
            "^Mixxx$",
            "getwindowgeometry",
        ).decode()
        if "1024x600" not in geometry:
            raise AssertionError(geometry)
        return geometry

    @classmethod
    def stop(cls):
        # Collect evidence even if startup or a test fails, then remove only our container.
        try:
            cls.verify_owner()
        except (subprocess.SubprocessError, AssertionError, OSError):
            return  # Docker unavailable or startup never created our container.
        try:
            for args, name in [
                (("docker", "logs", cls.container), "container.log"),
                (
                    ("docker", "exec", cls.container, "cat", "/tmp/audio.log"),
                    "audio.log",
                ),
            ]:
                try:
                    result = subprocess.run(
                        args, capture_output=True, timeout=20
                    )
                    (cls.artifacts / name).write_bytes(
                        result.stdout + result.stderr
                    )
                except subprocess.TimeoutExpired as error:
                    (cls.artifacts / name).write_text(str(error))
        finally:
            try:
                subprocess.run(
                    ["docker", "exec", cls.container, "pkill", "-9", "mixxx"],
                    capture_output=True,
                    timeout=20,
                )
            finally:
                subprocess.run(
                    ["docker", "rm", "-f", cls.container],
                    capture_output=True,
                    timeout=20,
                )

    @classmethod
    def verify_owner(cls):
        owner = (
            command(
                "docker",
                "inspect",
                "--format",
                '{{ index .Config.Labels "us.bitedj.test.worktree" }}',
                cls.container,
            )
            .decode()
            .strip()
        )
        branch = (
            command(
                "docker",
                "inspect",
                "--format",
                '{{ index .Config.Labels "us.bitedj.test.branch" }}',
                cls.container,
            )
            .decode()
            .strip()
        )
        current_branch = (
            command("git", "-C", str(ROOT), "branch", "--show-current")
            .decode()
            .strip()
        )
        if owner != str(ROOT) or branch != current_branch:
            raise AssertionError(
                f"Unexpected container owner: {owner} / {branch}"
            )

    def tearDown(self):
        name = self.id().rsplit(".", 1)[-1]
        self.inside("scrot", "-o", "/tmp/screen.png")
        command(
            "docker",
            "cp",
            f"{self.container}:/tmp/screen.png",
            str(self.artifacts / f"{name}.png"),
        )

    def click(self, x, y):
        # GUI_TESTING.md requires allowing the fullscreen menu to settle.
        self.inside(
            "xdotool",
            "mousemove",
            str(x),
            str(y),
            "sleep",
            "0.2",
            "click",
            "1",
        )

    def assert_selected_tab(self, selected):
        # The topbar's five equal cells have a black selected background and
        # purple inactive background (style.qss). Sample inside each button,
        # above its text: y=10, cell center x=(i+0.5)*1024/5.
        self.inside("scrot", "-o", "/tmp/screen.png")
        rgb = self.inside(
            "ffmpeg",
            "-v",
            "error",
            "-i",
            "/tmp/screen.png",
            "-f",
            "rawvideo",
            "-pix_fmt",
            "rgb24",
            "pipe:1",
        )
        self.assertEqual(1024 * 600 * 3, len(rgb))
        for index in range(5):
            offset = (10 * 1024 + int((index + 0.5) * 1024 / 5)) * 3
            pixel = tuple(rgb[offset : offset + 3])
            self.assertEqual(
                (0, 0, 0) if index == selected else (133, 94, 167),
                pixel,
                f"tab {index}, selected={selected}",
            )

    def test_navigation_and_reselect(self):
        for index, x in enumerate((100, 300, 500, 700, 950)):
            with self.subTest(tab=index):
                self.click(x, 20)
                eventually(lambda: self.assert_selected_tab(index))
                self.click(x, 20)
                eventually(lambda: self.assert_selected_tab(index))
        self.click(100, 20)

    def audio_rms(self):
        pcm = self.inside(
            "ffmpeg",
            "-v",
            "error",
            "-rw_timeout",
            "5000000",
            "-i",
            "http://localhost:8000/stream.mp3",
            "-ss",
            "0.3",
            "-t",
            "0.4",
            "-f",
            "s16le",
            "-ac",
            "1",
            "-ar",
            "44100",
            "pipe:1",
        )
        self.assertGreaterEqual(len(pcm), 30000)
        samples = array.array("h", pcm)
        if sys.byteorder != "little":
            samples.byteswap()
        return math.sqrt(
            sum(value * value for value in samples) / len(samples)
        )

    def test_playback_reaches_audio_output_and_stops(self):
        self.click(100, 20)
        eventually(lambda: self.assert_selected_tab(0))
        self.assertLess(self.audio_rms(), 30, "Fresh deck should be silent")
        # en_US.kbd.cfg maps D to Channel1 play; focus the main window first.
        self.inside("xdotool", "key", "d")
        try:
            eventually(
                lambda: self.assertGreater(self.audio_rms(), 500), timeout=20
            )
        finally:
            self.inside("xdotool", "key", "d")
        eventually(lambda: self.assertLess(self.audio_rms(), 30), timeout=20)

    def test_web_and_audio_transport(self):
        for port, path in (
            (6080, "/vnc.html"),
            (6080, "/core/rfb.js"),
            (8000, "/"),
        ):
            with self.subTest(path=path):
                body = self.inside(
                    "curl",
                    "--fail",
                    "--silent",
                    "--show-error",
                    "--max-time",
                    "5",
                    f"http://localhost:{port}{path}",
                )
                self.assertGreater(len(body), 100)
        # Decode the stream, rather than treating changing MP3 bytes as proof of DSP.
        pcm = self.inside(
            "ffmpeg",
            "-v",
            "error",
            "-rw_timeout",
            "5000000",
            "-i",
            "http://localhost:8000/stream.mp3",
            "-t",
            "0.25",
            "-f",
            "s16le",
            "-ac",
            "1",
            "-ar",
            "44100",
            "pipe:1",
        )
        self.assertGreaterEqual(len(pcm), 20000)
