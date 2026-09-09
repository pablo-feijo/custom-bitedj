"""Check a lossless 1024x600 Night-theme touch-cycle recording for drawer jumps.

Run inside the owned GUI container, where ffmpeg is available. Capture with
x11grab at 60 fps (draw_mouse=0) and FFV1 while capture_touch_cycle.py runs.
The header's left border is at x=145, y=456..497 in this fixture; its color is
#32323c, or #855ea7 in Pad FX. Ignore startup frames before the drawer appears.
"""
import argparse
import collections
import json
import subprocess
import sys


def check(path, x, top, bottom):
    process = subprocess.Popen([
        'ffmpeg', '-v', 'error', '-i', path,
        '-vf', f'format=rgb24,crop=1:600:{x}:0',
        '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-',
    ], stdout=subprocess.PIPE)
    colors = {bytes.fromhex('32323c'), bytes.fromhex('855ea7')}
    positions = collections.Counter()
    unstable = []
    frames = checked = 0
    started = False
    while True:
        frame = process.stdout.read(1800)
        if not frame:
            break
        if len(frame) != 1800:
            raise RuntimeError('Incomplete video frame')
        runs = []
        for y in range(200, 580):
            if frame[y * 3:y * 3 + 3] in colors:
                if runs and runs[-1][-1] == y - 1:
                    runs[-1].append(y)
                else:
                    runs.append([y])
        borders = tuple((run[0], run[-1]) for run in runs if 20 <= len(run) <= 60)
        started = started or bool(borders)
        if started:
            checked += 1
            positions[str(borders)] += 1
            if borders != ((top, bottom),):
                unstable.append(frames)
        frames += 1
    if process.wait() != 0:
        raise RuntimeError('ffmpeg could not decode the recording')
    result = dict(frames=frames, checked_frames=checked,
                  header_positions=dict(positions), unstable_frames=unstable)
    print(json.dumps(result, indent=2))
    return checked > 0 and not unstable


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('recording')
    parser.add_argument('--x', type=int, default=145)
    parser.add_argument('--top', type=int, default=456)
    parser.add_argument('--bottom', type=int, default=497)
    args = parser.parse_args()
    sys.exit(0 if check(args.recording, args.x, args.top, args.bottom) else 1)
