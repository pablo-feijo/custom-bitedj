"""Run inside the owned GUI container after starting with virtual_portmidi.so."""
import json, math, socket, struct, subprocess, time, wave
from pathlib import Path

out=Path('/tmp/padfx-audio'); out.mkdir(exist_ok=True)
sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
def midi(status,note,value):
    sock.sendto(bytes([status,note,value]),('127.0.0.1',21940))
def capture(name):
    path=out/(name+'.wav')
    subprocess.run(['ffmpeg','-nostdin','-v','error','-y','-f','pulse','-i','auto_null.monitor','-t','2','-ar','44100','-ac','2','-c:a','pcm_s16le',str(path)],check=True)
    with wave.open(str(path),'rb') as f:
        rate=f.getframerate(); samples=struct.unpack('<'+'h'*(f.getnframes()*2),f.readframes(f.getnframes()))
    mono=[s/32768 for s in samples[::2]][rate//2:]
    def band(hz):
        a=sum(v*math.cos(2*math.pi*hz*i/rate) for i,v in enumerate(mono))
        b=sum(v*math.sin(2*math.pi*hz*i/rate) for i,v in enumerate(mono))
        return 2*math.hypot(a,b)/len(mono)
    return {'rms':math.sqrt(sum(v*v for v in mono)/len(mono)), 'peak':max(map(abs,mono)), '1000Hz':band(1000),'8000Hz':band(8000)}

# Deck 1 play, followed by the actual DDJ-400 pad press and note-off.
midi(0x90,0x0b,127);midi(0x90,0x0b,0);time.sleep(1)
results={}
try:
    results['dry']=capture('dry')
    midi(0x97,0x10,127);time.sleep(.4)
    results['sweep']=capture('sweep')
    midi(0x87,0x10,0);time.sleep(.4)
    results['released']=capture('released')
finally:
    midi(0x87,0x10,0)
    midi(0x90,0x0b,127);midi(0x90,0x0b,0)
(out/'measurements.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results,indent=2))
assert results['dry']['rms']>.005, 'Dry audio must be audible, not silence'
assert results['sweep']['8000Hz'] < results['dry']['8000Hz']*.5, 'Sweep must attenuate the high-frequency band'
assert .9 < results['released']['rms']/results['dry']['rms'] < 1.1, 'Release must restore dry audio'
print('PASS: real MIDI mapping -> private DSP -> captured PulseAudio, release restores dry signal')
