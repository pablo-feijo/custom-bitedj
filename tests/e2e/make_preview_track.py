#!/usr/bin/env python3
"""Create a short, asymmetric synthetic track for replacement/cold-preview checks."""
import array
import math
from pathlib import Path
import sys
import wave

output = Path(sys.argv[1])
output.parent.mkdir(parents=True, exist_ok=True)
rate = 44100
pcm = array.array('h')
for frame in range(rate * 16):
    t = frame / rate
    # Four visibly different sections; each channel has a distinct envelope.
    section = int(t // 4)
    envelope = .1 + .8 * math.exp(-(t % .5) * (4 + section * 4))
    left = math.sin(2 * math.pi * (110, 440, 1800, 6000)[section] * t)
    right = math.sin(2 * math.pi * (220, 660, 3600, 8000)[section] * t)
    pcm.extend((int(20000 * left * envelope), int(12000 * right * envelope)))
if sys.byteorder != 'little':
    pcm.byteswap()
with wave.open(str(output), 'wb') as track:
    track.setparams((2, 2, rate, 0, 'NONE', 'not compressed'))
    track.writeframes(pcm.tobytes())
print(output)
