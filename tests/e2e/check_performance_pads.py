#!/usr/bin/env python3
"""Verify performance-pad hit targets and release feedback in the owned GUI.

Start with a fresh Night-mode instance and no controller. Native routing tests
cover the matching control/audio actions; this checks the real skin hit areas.
"""
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
owner = subprocess.run(['bash', '-c',
    'source scripts/test/gui-test-settings.sh; verify_test_instance_owner || exit; printf "%s" "$CONTAINER_NAME"'],
    cwd=ROOT, check=True, capture_output=True, text=True)
container = owner.stdout.strip()
out = ROOT / 'test-results' / container / 'performance-touch'
out.mkdir(parents=True, exist_ok=True)

def inside(*args):
    return subprocess.run(['docker', 'exec', '-e', 'DISPLAY=:99', container, *args],
                          check=True, capture_output=True).stdout

def click(x, y):
    inside('xdotool', 'mousemove', str(x), str(y), 'click', '1')
    time.sleep(.15)

def capture(name):
    inside('scrot', '-o', '/tmp/performance-touch.png')
    subprocess.run(['docker', 'cp', f'{container}:/tmp/performance-touch.png', str(out / (name + '.png'))], check=True)
    return inside('ffmpeg', '-v', 'error', '-i', '/tmp/performance-touch.png',
                  '-vf', 'crop=1016:92:4:500', '-f', 'rawvideo', '-pix_fmt', 'rgb24', 'pipe:1')

click(100, 20)
for deck, chip in ((1, 60), (2, 572)):
    click(chip, 462)
    click(944, 476)  # Memory
    click(944, 476)  # Beat Jump
    for mode in ('beat-jump', 'pad-fx', 'pad-fx-2', 'beat-loop'):
        before = capture(f'deck-{deck}-{mode}')
        # Pad 2 is forward Jump / held Sweep / held 1/2-beat Roll.
        inside('xdotool', 'mousemove', '385', '525', 'mousedown', '1')
        time.sleep(.2)
        pressed = capture(f'deck-{deck}-{mode}-pressed')
        assert pressed != before, f'{mode}: pressing must highlight its pad'
        inside('xdotool', 'mouseup', '1')
        time.sleep(.2)
        assert capture(f'deck-{deck}-{mode}-released') == before, f'{mode}: release must clear highlight'
        click(944, 476)
    click(994, 476)
print('PASS: both decks and all performance modes accept press/release in the native drawer')
print(out)
