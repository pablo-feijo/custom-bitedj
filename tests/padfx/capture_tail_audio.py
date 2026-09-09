import socket,subprocess,time,wave,struct,json,math
from pathlib import Path
out=Path('/tmp/padfx-audio')
s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
def midi(a,b,c):s.sendto(bytes([a,b,c]),('127.0.0.1',21940))
def play():midi(0x90,0x0b,127);midi(0x90,0x0b,0)
def record(name):
 p=out/(name+'.wav');subprocess.run(['ffmpeg','-nostdin','-v','error','-y','-f','pulse','-i','auto_null.monitor','-t','2','-ar','44100','-ac','2','-c:a','pcm_s16le',str(p)],check=True)
 with wave.open(str(p),'rb') as f: data=struct.unpack('<'+'h'*(f.getnframes()*2),f.readframes(f.getnframes()))
 data=[x/32768 for x in data[::2]]
 def rms(seq):return math.sqrt(sum(x*x for x in seq)/len(seq))
 return {'rms':rms(data),'first_half_rms':rms(data[:44100]),'last_half_rms':rms(data[44100:])}
r={}
for name,note in [('echo',0x14),('reverb',0x16)]:
 play();time.sleep(.5)
 r[name+'_dry']=record(name+'_dry')
 midi(0x97,note,127);time.sleep(.5)
 r[name+'_held']=record(name+'_held')
 # Stop the source while the pad's DSP remains enabled, then release the pad.
 play();midi(0x87,note,0)
 r[name+'_tail']=record(name+'_tail')
 time.sleep(4)
(out/'additional-measurements.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
for name in ('echo','reverb'):
 assert r[name+'_dry']['rms']>.005, name+' baseline must be audible'
 tail=r[name+'_tail']
 assert tail['first_half_rms']>.001, name+' must leave a captured tail'
 assert tail['last_half_rms']<tail['first_half_rms']*.1, name+' tail must decay'
print('PASS: Echo and Reverb leave real, decaying output tails after MIDI release')
