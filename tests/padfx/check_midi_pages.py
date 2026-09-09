"""Run inside an owned GUI with the virtual DDJ-400 and fresh runtime state.

Check displayed options after real mode/bank MIDI messages, including Shift
release. Never start playback. Save screenshots and comparisons in /tmp.
"""
import os
from pathlib import Path
import socket
import subprocess
import time

out = Path('/tmp/midi-page-check')
out.mkdir(exist_ok=True)
env = dict(os.environ, DISPLAY=':99')
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def midi(status, note, value):
    sock.sendto(bytes([status, note, value]), ('127.0.0.1', 21940))

def tap(status, note):
    midi(status, note, 127)
    midi(status, note, 0)

def select(deck, note):
    tap(0x8f + deck, note)
    time.sleep(.2)

def shift(deck, down):
    midi(0x8f + deck, 0x3f, 127 if down else 0)
    time.sleep(.15)

def capture(name):
    time.sleep(.2)
    path = out / (name + '.png')
    subprocess.run(['scrot', '-o', str(path)], env=env, check=True)
    return subprocess.run(['ffmpeg', '-v', 'error', '-i', str(path), '-vf',
        'crop=1016:140:4:454', '-f', 'rawvideo', '-pix_fmt', 'rgb24', 'pipe:1'],
        check=True, capture_output=True).stdout

subprocess.run(['xdotool', 'mousemove', '100', '20', 'click', '1'], env=env, check=True)
for deck in (1, 2):
    select(deck, 0x6d)
    loop = capture(f'd{deck}-loop')
    shift(deck, True)
    assert capture(f'd{deck}-loop-shift') != loop
    shift(deck, False)
    assert capture(f'd{deck}-loop-release') == loop

    select(deck, 0x1e)
    fx1 = capture(f'd{deck}-fx1')
    shift(deck, True)
    select(deck, 0x6b)
    fx2 = capture(f'd{deck}-fx2-selected')
    shift(deck, False)
    assert capture(f'd{deck}-fx2-release') == fx2, 'FX 2 must survive selecting Shift release'
    assert fx2 != fx1
    subprocess.run(['xdotool', 'mousemove', '944', '476', 'click', '1'], env=env, check=True)
    assert capture(f'd{deck}-touch-next-loop') == loop, 'Touch navigation must continue from the MIDI-selected page'

    select(deck, 0x20)
    normal = capture(f'd{deck}-jump-normal')
    def bank(note, count, name):
        select(deck, 0x20)  # Reset the independent triple-Shift close gesture.
        shift(deck, True)
        for _ in range(count):
            tap(0x96 + deck * 2, note)
            time.sleep(.15)
        shift(deck, False)
        return capture(f'd{deck}-jump-{name}')
    small = bank(0x26, 1, 'small')
    large = bank(0x27, 2, 'large')
    assert len({normal, small, large}) == 3, 'Jump options must show each selected size bank'
    assert bank(0x26, 1, 'restored') == normal, 'Returning to the middle bank must restore its options'
    select(deck, 0x1b)

(out / 'result.txt').write_text('PASS: both decks, Loop Shift/release, FX 1/2, FX 2 Shift release, MIDI-to-touch navigation, and all Jump banks.\n')
print((out / 'result.txt').read_text())
