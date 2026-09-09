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

def tap_next():
    # Separate Next button, restored from the integration layout.
    subprocess.run(['xdotool', 'mousemove', '944', '477', 'click', '1'], env=env, check=True)

for deck in (1, 2):
    select(deck, 0x1b)
    capture(f'deck-{deck}-hot-cues')
    for mode in ('memory', 'beat-jump', 'pad-fx', 'pad-fx-2', 'beat-loop', 'hot-cues-return'):
        tap_next()
        capture(f'deck-{deck}-{mode}')
select(1, 0x1e)
tap_next()
capture('controller-fx-then-touch-fx2')
# A held touch must advance once, and release must leave Memory selected.
select(1, 0x1b)
subprocess.run(['xdotool', 'mousemove', '944', '477', 'mousedown', '1'], env=env, check=True)
time.sleep(.6)
capture('held-memory')
subprocess.run(['xdotool', 'mouseup', '1'], env=env, check=True)
capture('released-memory')
tap_next()
capture('next-beat-jump')
for deck in (1, 2):
    select(deck, 0x1b)
    for mode in ('beat-loop', 'pad-fx-2', 'pad-fx', 'beat-jump', 'memory', 'hot-cues'):
        subprocess.run(['xdotool', 'mousemove', '116', '477', 'click', '1'], env=env, check=True)
        capture(f'deck-{deck}-previous-{mode}')
print(out)
