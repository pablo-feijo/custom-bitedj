#!/usr/bin/env python3
"""Check both touch directions in the owned GUI after a fresh application start.

No controller required. Leaves both decks on Hot Cues with the drawer closed.
"""
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
result = subprocess.run(['bash', '-c',
    'source scripts/test/gui-test-settings.sh; verify_test_instance_owner || exit; printf "%s" "$CONTAINER_NAME"'],
    cwd=ROOT, check=True, capture_output=True, text=True)
container = result.stdout.strip()
out = ROOT / 'test-results' / container / 'touch-drawer'
out.mkdir(parents=True, exist_ok=True)

def inside(*args):
    return subprocess.run(['docker', 'exec', '-e', 'DISPLAY=:99', container, *args],
                          check=True, capture_output=True).stdout

def click(x, y):
    inside('xdotool', 'mousemove', str(x), str(y), 'click', '1')
    time.sleep(.15)

def capture(name):
    inside('xdotool', 'mousemove', '1023', '449')
    inside('scrot', '-o', '/tmp/touch-drawer.png')
    subprocess.run(['docker', 'cp', f'{container}:/tmp/touch-drawer.png', str(out / (name + '.png'))], check=True)
    return inside('ffmpeg', '-v', 'error', '-i', '/tmp/touch-drawer.png', '-vf',
                  'crop=770:44:144:454', '-f', 'rawvideo', '-pix_fmt', 'rgb24', 'pipe:1')

click(100, 20)
for deck, chip in ((1, 60), (2, 572)):
    click(chip, 462)
    states = [capture(f'deck-{deck}-hot-cues')]
    for mode in ('memory', 'beat-jump', 'pad-fx', 'pad-fx-2', 'beat-loop'):
        click(944, 476)
        states.append(capture(f'deck-{deck}-{mode}'))
    assert len(set(states)) == 6, 'Six distinct mode labels must be reachable'
    click(944, 476)
    assert capture(f'deck-{deck}-next-wrap') == states[0], 'Next must wrap'
    for i in (5, 4, 3, 2, 1, 0):
        click(116, 476)
        assert capture(f'deck-{deck}-previous-{i}') == states[i], 'Previous must reverse Next'
    click(994, 476)
print('PASS: both decks, all six touch modes, next wrap and full reverse cycle')
print(out)
