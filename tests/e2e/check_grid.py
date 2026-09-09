#!/usr/bin/env python3
"""Verify per-deck grid controls in an owned 1024×600 GUI.
Requires both synthetic tracks loaded, analyzed and paused in Night mode,
with the cue drawer closed. Edits only the synthetic test tracks' grids.
"""
import subprocess,time
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
import os
os.chdir(ROOT)
C = subprocess.check_output(['bash', '-c',
    'source scripts/test/gui-test-settings.sh; verify_test_instance_owner || exit; printf "%s" "$CONTAINER_NAME"'], text=True).strip()
OUT = ROOT / 'test-results' / C / 'grid-check'
OUT.mkdir(parents=True, exist_ok=True)
def inside(*a): return subprocess.check_output(['docker','exec','-e','DISPLAY=:99',C,*a])
def click(x,y,n=1):
 inside('xdotool','mousemove',str(x),str(y),'click','--repeat',str(n),'--delay','40','1','mousemove','1023','599');time.sleep(.3)
def shot(name):
 inside('scrot','-o','/tmp/grid-check.png');subprocess.run(['docker','cp',C+':/tmp/grid-check.png',str(OUT / (name+'.png'))],check=True,stdout=subprocess.DEVNULL)
 return inside('ffmpeg','-v','error','-i','/tmp/grid-check.png','-f','rawvideo','-pix_fmt','rgb24','pipe:1')
def band(im,d):return b''.join(im[(y*1024+126)*3:(y*1024+840)*3] for y in (range(40,244) if d==1 else range(246,448)))
click(100,20);click(997,70)
for d,y in [(1,146),(2,314)]:
 a=shot('grid-shift-base'+str(d));click(892,y,10);b=shot('grid-earlier'+str(d))
 assert band(a,d)!=band(b,d), 'Earlier must move its deck grid'
 assert band(a,3-d)==band(b,3-d), 'Earlier changed opposite deck'
 click(976,y,10);c=shot('grid-later'+str(d))
 assert band(b,d)!=band(c,d), 'Later must move its deck grid'
 assert band(b,3-d)==band(c,3-d), 'Later changed opposite deck'
 # Offset the grid and align to the stationary playhead.
 click(892,y,10);a=shot('grid-set-before'+str(d));click(934,y+48);b=shot('grid-set-after'+str(d))
 assert band(a,d)!=band(b,d),'Set here must move grid'
 assert band(a,3-d)==band(b,3-d),'Set here changed opposite deck'
print('PASS: Earlier, Later and Set here move only the chosen deck grid on both decks')

for d,y in [(1,140),(2,340)]:
    a=shot('drag-before'+str(d))
    inside('xdotool','mousemove','600',str(y),'mousedown','1','sleep','.2','mousemove','570',str(y),'sleep','.3','mouseup','1','mousemove','1023','599')
    time.sleep(.3)
    b=shot('drag-after'+str(d))
    assert band(a,d)!=band(b,d),'Grid mode must allow positioning by drag'
    assert band(a,3-d)==band(b,3-d),'Drag moved opposite deck'
click(871,70)
a=shot('normal-before-drag')
inside('xdotool','mousemove','600','140','mousedown','1','sleep','.2','mousemove','570','140','sleep','.3','mouseup','1','mousemove','1023','599')
time.sleep(.3)
b=shot('normal-after-drag')
assert band(a,1)==band(b,1),'Leaving Grid must restore seek-disabled behavior'
click(997,70)
print('PASS: Grid permits per-deck dragging and FX restores normal interaction')
