"""Run in an owned 1024x600 GUI instance with virtual_portmidi.so loaded.
Captures controller-selected modes; sends no pad notes or playback commands.
"""
import os
import socket
import subprocess
import time
from pathlib import Path

out = Path('/tmp/controller-pad-display')
out.mkdir(exist_ok=True)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
env = dict(os.environ, DISPLAY=':99')
def midi(status, note, value):
    sock.sendto(bytes([status, note, value]), ('127.0.0.1', 21940))
def select(deck, note):
    midi(0x8f + deck, note, 127)
    midi(0x8f + deck, note, 0)
def capture(name):
    time.sleep(.4)
    target = out / (name + '.png')
    target.unlink(missing_ok=True)
    subprocess.run(['scrot', str(target)], env=env, check=True)

for deck in (1, 2):
    for note, name in ((0x1e, 'pad-fx'), (0x6b, 'pad-fx-2'), (0x20, 'beat-jump'), (0x6d, 'beat-loop'), (0x1b, 'hot-cues')):
        select(deck, note)
        capture(f'deck-{deck}-{name}')
select(1, 0x1e)
midi(0x90, 0x3f, 127)
capture('deck-1-pad-fx-shift')
midi(0x90, 0x3f, 0)
select(1, 0x22)
capture('other-mode-closed')
select(1, 0x1e)
print(out)
