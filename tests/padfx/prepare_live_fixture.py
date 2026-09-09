"""Prepare only an isolated test instance's stopped config and synthetic audio."""
import math,re,struct,sys,wave
from pathlib import Path
root,config=map(Path,sys.argv[1:])
p=config/'mixxx.cfg';s=p.read_text()
def setval(section,key,value):
 global s
 h='['+section+']'
 if h not in s:s+='\n'+h+'\n'
 start=s.index(h)+len(h);end=s.find('\n[',start)
 if end<0:end=len(s)
 body=re.sub(r'(?m)^'+re.escape(key)+r' .*\n?', '',s[start:end])
 s=s[:start]+body.rstrip()+'\n'+key+' '+str(value)+'\n'+s[end:]
setval('Controller','DDJ-400',1)
setval('ControllerPreset','DDJ-400','Pioneer-DDJ-400.midi.xml')
for slot,effect in ((0,1),(4,4),(6,6)):
 for field,value in (('effect',effect),('beat',0),('strength',4),('hold',0)):
  setval('PadFX_v1',f'd1_s{slot}_{field}',value)
p.write_text(s)
with wave.open(str(root/'test-music/PadFX_Calibration.wav'),'wb') as w:
 w.setparams((2,2,44100,0,'NONE','not compressed'))
 data=b''.join(struct.pack('<hh',v,v) for i in range(44100) for v in [int(32767*.05*(math.sin(2*math.pi*1000*i/44100)+math.sin(2*math.pi*8000*i/44100)))])
 for _ in range(240):w.writeframesraw(data)
