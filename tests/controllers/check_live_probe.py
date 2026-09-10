"""Exercise real DDJ-400 MIDI dispatch with isolated settings and synthetic tracks.
Run on Linux: check_live_probe.py LOG SENDER ALSA_CLIENT ALSA_PORT.
The sender is built from send_alsa_midi.c; the preset is prepare_live_probe.py's
output. A live physical DDJ-400 can remain attached. This injects MIDI; it cannot
measure a person's physical gestures or judge speaker sound.
"""
import json
from pathlib import Path
import subprocess
import sys
import time

log, sender, client, port = sys.argv[1:]

def send(status, note, value):
    subprocess.run([sender, client, port, str(status), str(note), str(value)], check=True)

def command(value):
    send(0xbf, 0x7e, value)
    time.sleep(.35)

def snapshot():
    for line in reversed(Path(log).read_text(errors='replace').splitlines()):
        if 'BITEDJ_PROBE {' in line:
            return json.loads(line.split('BITEDJ_PROBE ', 1)[1])
    raise AssertionError('No probe telemetry')

def state(deck):
    return snapshot()['d' + str(deck)]

def wait_for(check, timeout=8):
    until = time.monotonic() + timeout
    while time.monotonic() < until:
        if check():
            return
        time.sleep(.1)
    raise AssertionError('Timed out: ' + str(snapshot()))

def press(deck, note):
    send(0x8f + deck, note, 127)
    send(0x8f + deck, note, 0)
    time.sleep(.3)

wait_for(lambda: all(state(d)['file_bpm'] > 0 for d in (1, 2)), 60)
for panel in (0, 1, 2, 3):
    for deck in (1, 2):
        command(10)
        command(panel)
        before = state(deck)
        send(0x8f + deck, 0x67, 127)
        for _ in range(8):
            send(0xaf + deck, 0x29, 66)
            time.sleep(.03)
        send(0x8f + deck, 0x67, 0)
        time.sleep(.5)
        after = state(deck)
        if panel == 3:
            assert abs(after['playposition'] - before['playposition']) < .0001, (panel, deck, before, after)
            assert after['beat_next'] != before['beat_next'], (panel, deck, before, after)
        else:
            assert after['playposition'] > before['playposition'] + .001, (panel, deck, before, after)
        assert after['scratch2_enable'] == 0
print('PASS: Shift jog searches FX/Key/Jump and edits Grid on both decks', flush=True)

for deck in (1, 2):
    durations = []
    for setting in (21, 22, 23):
        command(10)
        command(setting)
        send(0x8f + deck, 0x36, 127)
        for _ in range(12):
            send(0xaf + deck, 0x22, 60)
            time.sleep(.02)
        released = time.monotonic()
        send(0x8f + deck, 0x36, 0)
        time.sleep(.15)
        wait_for(lambda: state(deck)['scratch2_enable'] == 0, 12)
        durations.append(time.monotonic() - released)
        assert state(deck)['play'] == 0
    # ALSA delivery/telemetry adds latency and the thrown speed is not exactly 1x.
    # Compare settings here; the native timing test controls the initial rate.
    assert durations[1] > durations[0] * 1.5, durations
    assert durations[2] > durations[1] * 1.4, durations
    print('PASS: deck', deck, 'Off/Short/Long coast seconds', durations, flush=True)

for deck in (1, 2):
    command(10)
    command(20)
    send(0x8f + deck, 0x36, 127)
    time.sleep(.2)
    assert state(deck)['scratch2_enable'] == 0
    send(0x8f + deck, 0x36, 0)
    command(30)
    tempo = state(deck)['bpm']
    press(deck, 0x58)
    wait_for(lambda: all(state(d)['sync_enabled'] == 1 for d in (1, 2)))
    assert all(abs(state(d)['bpm'] - tempo) < .01 for d in (1, 2)), snapshot()
    press(deck, 0x58)
    wait_for(lambda: all(state(d)['sync_enabled'] == 0 for d in (1, 2)))
    assert all(abs(state(d)['bpm'] - tempo) < .01 for d in (1, 2)), snapshot()
    for _ in range(15):
        send(0xaf + deck, 0x22, 68)
        time.sleep(.02)
    time.sleep(.7)
    def difference():
        s = snapshot()
        return (s['d1']['beat_distance'] - s['d2']['beat_distance'] + .5) % 1 - .5
    offset = difference()
    assert abs(offset) > .015, snapshot()
    time.sleep(1.5)
    assert abs(difference() - offset) < .035, (offset, difference())
    print('PASS: deck', deck, 'leads Sync; off retains BPM and jog phase offset', flush=True)
command(10)
print('PASS: live controller probe complete', flush=True)
