"""Capture touch cycling in the owned 1024x600 virtual-controller GUI."""
import os
from pathlib import Path
import socket
import subprocess
import time

out = Path('/tmp/touch-cycle')
out.mkdir(exist_ok=True)
env = dict(os.environ, DISPLAY=':99')
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def select(deck, note):
    for value in (127, 0):
        sock.sendto(bytes([0x8f + deck, note, value]), ('127.0.0.1', 21940))
    time.sleep(.2)

def capture(name):
    target = out / (name + '.png')
    target.unlink(missing_ok=True)
    time.sleep(.2)
    subprocess.run(['scrot', str(target)], env=env, check=True)

def tap_header():
    # Verified separate Next button: x=920..967, y=454..497.
    subprocess.run(['xdotool', 'mousemove', '944', '476', 'click', '1'], env=env, check=True)

for deck in (1, 2):
    select(deck, 0x1b)
    capture(f'deck-{deck}-hot-cues')
    for mode in ('memory', 'beat-jump', 'pad-fx', 'beat-loop', 'hot-cues-return'):
        tap_header()
        capture(f'deck-{deck}-{mode}')
select(1, 0x1e)
tap_header()
capture('controller-fx-then-touch-loop')
print(out)

subprocess.run(['xdotool', 'mousemove', '116', '476', 'click', '1'], env=env, check=True)
capture('touch-previous-back-to-fx')
