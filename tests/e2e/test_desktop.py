"""Real desktop + audio smoke tests in a disposable container, with no host ports."""

import array
from collections import Counter
import math
import json
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
        # The startup window can disappear between search and activation.
        # Retry discovery as well as activation until the main window is stable.
        eventually(lambda: cls.inside(
            "xdotool", "search", "--onlyvisible", "--name", "^Mixxx$",
            "windowactivate", "--sync"), timeout=60)
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

    def test_autoplay_queue_controls(self):
        def pixel(x, y):
            self.inside("scrot", "-o", "/tmp/queue-control.png")
            rgb = self.inside("ffmpeg", "-v", "error", "-i", "/tmp/queue-control.png",
                "-f", "rawvideo", "-pix_fmt", "rgb24", "pipe:1")
            return tuple(rgb[(y * 1024 + x) * 3:(y * 1024 + x) * 3 + 3])

        # Fresh instance: no Auto DJ queue, Computer -> Quick Links -> Music.
        # The Info dashboard has native child controls and may still be settling
        # after the class startup probe selected Settings. Confirm the actual
        # page transition before targeting Browse coordinates.
        def browse_ready():
            self.click(300, 20)
            self.assert_selected_tab(1)
        eventually(browse_ready, timeout=60)
        self.click(120, 55)
        self.click(150, 83)
        self.click(170, 111)
        time.sleep(1)  # Browse loads its directory model asynchronously.
        self.click(892, 56)
        eventually(lambda: self.assertEqual(pixel(10, 10), (26, 17, 0)))
        self.click(512, 20)  # Dismiss the in-skin notice; kiosk rejects Qt dialogs.
        eventually(lambda: self.assert_selected_tab(1))
        self.assertLess(self.audio_rms(), 30, "Empty queue must not start a deck")

        def queue_count():
            return int(self.inside("python3", "-c",
                "import sqlite3; c=sqlite3.connect('file:/root/.mixxx/mixxxdb.sqlite?mode=ro',uri=True); "
                "print(c.execute(\"select count(*) from PlaylistTracks where playlist_id="
                "(select id from Playlists where name='Auto DJ')\").fetchone()[0])"))

        self.click(600, 102)
        self.click(683, 56)  # Selected track.
        eventually(lambda: self.assertEqual(queue_count(), 1))
        eventually(lambda: self.assertEqual(pixel(10, 10), (26, 17, 0)))
        self.click(512, 20)
        self.click(780, 56)  # The full one-track fixture playlist.
        eventually(lambda: self.assertEqual(queue_count(), 2))
        eventually(lambda: self.assertEqual(pixel(10, 10), (26, 17, 0)))
        self.click(512, 20)
        self.click(598, 56)  # View Queue opens Auto DJ directly.

        header_off = pixel(10, 80)
        self.click(892, 56)
        try:
            eventually(lambda: self.assertEqual(pixel(850, 47), (22, 115, 71)))
            eventually(lambda: self.assertNotEqual(pixel(10, 80), header_off))
            eventually(lambda: self.assertGreater(self.audio_rms(), 500), timeout=20)
            self.click(100, 20)
            eventually(lambda: self.assertEqual(pixel(840, 105), (22, 115, 71)))
            self.click(300, 20)
            self.click(892, 56)
            eventually(lambda: self.assertNotEqual(pixel(850, 47), (22, 115, 71)))
            eventually(lambda: self.assertEqual(pixel(10, 80), header_off))
            self.assertGreater(self.audio_rms(), 500, "Auto Play off must leave playback running")
            self.click(100, 20)
            eventually(lambda: self.assertNotEqual(pixel(840, 105), (22, 115, 71)))
            self.inside("xdotool", "key", "d")
            eventually(lambda: self.assertLess(self.audio_rms(), 30), timeout=20)
            self.click(300, 20)
            # Remove the remaining queued track; both decks stay loaded.
            self.click(600, 102)
            self.inside("xdotool", "mousemove", "600", "102", "click", "3")
            self.click(646, 226)
            eventually(lambda: self.assertEqual(queue_count(), 0))
            self.click(892, 56)
            eventually(lambda: self.assertEqual(pixel(850, 47), (22, 115, 71)))
            eventually(lambda: self.assertGreater(self.audio_rms(), 500), timeout=20)
            self.assertEqual(queue_count(), 1, "Loaded decks should supply the empty queue")
            self.click(892, 56)

        finally:
            self.click(100, 20)
            self.inside("xdotool", "key", "d")
        eventually(lambda: self.assertLess(self.audio_rms(), 30), timeout=20)

    def test_z_saved_playlist_queue(self):
        # Seed a saved playlist in this disposable database, then restart so
        # the library discovers it through the normal startup path.
        self.verify_owner()
        self.inside("python3", "-c",
            "import sqlite3; c=sqlite3.connect('/root/.mixxx/mixxxdb.sqlite'); "
            "track=c.execute('select id from library limit 1').fetchone()[0]; "
            "c.execute(\"delete from PlaylistTracks where playlist_id=(select id from Playlists where name='Auto DJ')\"); "
            "p=c.execute(\"insert into Playlists(name,position,hidden,locked) values('Queue Test',1,0,0)\").lastrowid; "
            "c.executemany('insert into PlaylistTracks(playlist_id,track_id,position) values(?,?,?)',[(p,track,1),(p,track,2)]); c.commit()")
        command("docker", "restart", self.container, timeout=40)
        eventually(self.window_ready, timeout=60)
        eventually(lambda: self.inside(
            "xdotool", "search", "--onlyvisible", "--name", "^Mixxx$",
            "windowactivate", "--sync"), timeout=60)
        def ready():
            self.click(950, 20)
            self.assert_selected_tab(4)
        eventually(ready, timeout=60)
        def browse_ready():
            self.click(300, 20)
            self.assert_selected_tab(1)
        eventually(browse_ready)
        self.click(100, 55)  # Playlists exists because the saved playlist exists.
        self.click(100, 83)  # Queue Test.
        time.sleep(1)
        self.click(780, 56)  # Queue All, with no manual row selection.
        def queued():
            count = int(self.inside("python3", "-c",
                "import sqlite3; c=sqlite3.connect('file:/root/.mixxx/mixxxdb.sqlite?mode=ro',uri=True); "
                "print(c.execute(\"select count(*) from PlaylistTracks where playlist_id=(select id from Playlists where name='Auto DJ')\").fetchone()[0])"))
            self.assertEqual(count, 2)
        eventually(queued)
        self.click(512, 20)  # Dismiss the added-track count.
        self.click(600, 102)
        self.click(683, 56)  # Add one duplicate: entries must remain independent.
        self.click(512, 20)
        self.click(598, 56)
        def order():
            return json.loads(self.inside("python3", "-c",
                "import sqlite3,json; c=sqlite3.connect('file:/root/.mixxx/mixxxdb.sqlite?mode=ro',uri=True); "
                "print(json.dumps([r[0] for r in c.execute(\"select id from PlaylistTracks where playlist_id=(select id from Playlists where name='Auto DJ') order by position\")]))"))
        original = order()
        self.assertEqual(len(original), 3)
        self.assertLess(self.audio_rms(), 30, "Queuing a playlist must not start playback")
        self.click(600, 124)  # Second pending entry.
        self.click(683, 56)  # Move Up.
        eventually(lambda: self.assertEqual(order(), [original[1], original[0], original[2]]))
        self.click(683, 56)  # At top: no wraparound.
        self.assertEqual(order(), [original[1], original[0], original[2]])
        self.click(780, 56)  # Selection follows the entry back down.
        eventually(lambda: self.assertEqual(order(), original))
        self.click(780, 56)
        eventually(lambda: self.assertEqual(order(), [original[0], original[2], original[1]]))
        self.click(780, 56)  # At bottom: no wraparound.
        self.assertEqual(order(), [original[0], original[2], original[1]])
        self.click(892, 56)
        try:
            eventually(lambda: self.assertGreater(self.audio_rms(), 500), timeout=20)
            eventually(lambda: self.assertEqual(order(), [original[2], original[1]]))
            self.click(600, 146)  # Second entry; options row is visible while on.
            self.click(683, 56)
            eventually(lambda: self.assertEqual(order(), [original[1], original[2]]))
            self.click(780, 56)
            eventually(lambda: self.assertEqual(order(), [original[2], original[1]]))
            self.click(598, 56)  # Remove the selected queue entry, not its file.
            eventually(lambda: self.assertEqual(order(), [original[2]]))
            self.assertGreater(self.audio_rms(), 500, "Queue editing must not stop the playing deck")
        finally:
            self.click(892, 56)
            self.click(100, 20)
            self.inside("xdotool", "key", "d")
        eventually(lambda: self.assertLess(self.audio_rms(), 30), timeout=20)
        self.click(300, 20)
        self.click(600, 102)
        self.click(598, 56)  # Removing the final pending entry is safe while off.
        eventually(lambda: self.assertEqual(order(), []))
        remaining = json.loads(self.inside("python3", "-c",
            "import sqlite3,json; c=sqlite3.connect('file:/root/.mixxx/mixxxdb.sqlite?mode=ro',uri=True); "
            "print(json.dumps([c.execute('select count(*) from library').fetchone()[0], c.execute(\"select count(*) from PlaylistTracks where playlist_id=(select id from Playlists where name='Queue Test')\").fetchone()[0]]))"))
        self.assertEqual(remaining, [1, 2], "Queue removal must preserve the library and source playlist")

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

    def waveform_regions(self):
        self.inside("scrot", "-o", "/tmp/waveforms.png")
        regions = []
        # Verified Play geometry: scrolling lane above, compact deck summary below.
        # Sample the left half of the compact overview so the deliberate
        # remaining-time shade over its unplayed/right side does not alter the
        # waveform palette comparison at the freshly cued position.
        for crop in ("240:120:520:70", "220:28:20:555"):
            regions.append(self.inside(
                "ffmpeg", "-v", "error", "-i", "/tmp/waveforms.png",
                "-vf", "crop=" + crop, "-f", "rawvideo", "-pix_fmt", "rgb24", "pipe:1"))
        return regions

    def test_waveform_settings_refresh_paused_deck(self):
        def select(style_x, palette_x):
            self.click(950, 20)
            self.click(73, 60)
            self.click(style_x, 104)
            self.click(palette_x, 208)
            self.click(100, 20)
            eventually(lambda: self.assert_selected_tab(0))
            time.sleep(.4)
            return self.waveform_regions()

        original = select(872, 886)
        for style_x, palette_x in ((872, 970), (928, 886), (984, 886)):
            with self.subTest(style=style_x, palette=palette_x):
                changed = select(style_x, palette_x)
                for surface, before, after in zip(("Play", "deck overview"), original, changed):
                    self.assertEqual(len(before), len(after))
                    self.assertGreater(sum(a != b for a, b in zip(before, after)), 300,
                                       surface + " did not refresh while paused")
        restored = select(872, 886)
        for before, after in zip(original, restored):
            self.assertEqual(before, after, "Changing settings must not move or reload the track")

    def test_waveform_rgb_preview_matches_play_colors(self):
        # Constant tones give both resolutions identical frequency content.
        # Cover all bands so a stuck green overview cannot pass accidentally.
        self.verify_owner()
        def restart(frequency):
            # Mixxx is PID 1 in this disposable container. Restart the container,
            # not a second competing app process, after replacing its test tone.
            command("docker", "stop", "--time", "3", self.container, timeout=30)
            samples = array.array("h", (int(8000 * math.sin(2 * math.pi * frequency * n / 44100))
                                       for n in range(44100)))
            if sys.byteorder != "little":
                samples.byteswap()
            with wave.open(str(Path(self.workspace.name) / "tone.wav"), "wb") as track:
                track.setparams((1, 2, 44100, 0, "NONE", "not compressed"))
                # Different duration also invalidates the previous analysis cache.
                for _ in range(180 + frequency // 80):
                    track.writeframesraw(samples.tobytes())
            command("docker", "start", self.container)
            eventually(self.window_ready, timeout=60)
            eventually(lambda: self.inside(
                "xdotool", "search", "--onlyvisible", "--name", "^Mixxx$",
                "windowactivate", "--sync"), timeout=60)
            def ready():
                self.click(950, 20)
                self.assert_selected_tab(4)
            eventually(ready, timeout=60)
            eventually(lambda: dominant_color(self.waveform_regions()[1]), timeout=60)

        def dominant_color(pixels):
            colors = Counter()
            for offset in range(0, len(pixels), 3):
                rgb = tuple(pixels[offset:offset + 3])
                # Exclude background, neutral grid lines and time text.
                if max(rgb) > 100 and max(rgb) - min(rgb) > 18:
                    colors[tuple(value // 8 for value in rgb)] += 1
            self.assertTrue(colors, "No colored waveform pixels")
            color, count = colors.most_common(1)[0]
            self.assertGreater(count, 50, "Only marker/overlay pixels were found")
            return tuple(value * 8 + 4 for value in color)

        def compare():
            play, preview = map(dominant_color, self.waveform_regions())
            if palette == 886:
                expected = (80, 1000, 8000).index(frequency)
                self.assertEqual(max(range(3), key=lambda channel: play[channel]), expected,
                                 f"Wrong analyzed tone color: {frequency} Hz, {play}")
            # The compact overview deliberately shades the side represented by
            # the selected time mode with #40ffffff. Accept either an unshaded
            # sample or that exact source-over blend, but still compare the
            # underlying waveform palette to the scrolling renderer.
            alpha = 0x40
            unshaded = tuple(max(0, min(255,
                round((value * 255 - alpha * 255) / (255 - alpha))))
                for value in preview)
            divergence = min(
                max(abs(a - b) for a, b in zip(play, preview)),
                max(abs(a - b) for a, b in zip(play, unshaded)),
            )
            self.assertLessEqual(divergence, 40,
                                 f"RGB colors diverged: Play={play}, bottom={preview}, "
                                 f"unshaded={unshaded}")

        def select(style, palette):
            self.click(950, 20)
            self.click(73, 60)
            self.click(style, 104)
            self.click(palette, 208)
            def play_ready():
                self.click(100, 20)
                self.assert_selected_tab(0)
            eventually(play_ready)

        try:
            for frequency in (80, 1000, 8000):
                restart(frequency)
                for palette in (886, 970, 886):
                    with self.subTest(frequency=frequency, palette=palette):
                        select(872, palette)
                        eventually(compare, timeout=30)
                        for other_style in (928, 984):
                            select(other_style, palette)
                            select(872, palette)
                            eventually(compare)
                        command("docker", "cp", f"{self.container}:/tmp/waveforms.png",
                                str(self.artifacts / f"rgb-{frequency}-{palette}.png"))
        finally:
            restart(440)

    def test_novnc_modules_parse(self):
        # HTTP 200 alone does not prove the browser can execute these modules.
        modules = json.loads(self.inside(
            "python3", "-c",
            "from pathlib import Path; import json; "
            "r=Path('/usr/share/novnc'); "
            "print(json.dumps([str(p.relative_to(r)) for p in r.rglob('*.js')]))",
        ))
        self.assertIn("core/util/browser.js", modules)
        for module in modules:
            with self.subTest(module=module):
                source = self.inside("cat", "/usr/share/novnc/" + module)
                result = subprocess.run(
                    ["node", "--input-type=module", "--check"],
                    input=source, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    timeout=20,
                )
                self.assertEqual(0, result.returncode, result.stderr.decode())

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
