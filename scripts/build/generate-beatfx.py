#!/usr/bin/env python3
"""Generate original native approximations; see docs/BEAT_FX.md. No vendor DSP."""
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]

def effect(backend, **parameters):
    return backend, parameters

def echo(time=.5, feedback=.55, pingpong=0):
    return effect('echo', delay_time=time, feedback_amount=feedback,
                  pingpong_amount=pingpong, send_amount=(.5, 'LINKED'), quantize=1, triplet=0)

def pitch(value=.25, linked=False):
    return effect('pitchshift', pitch=(value, 'LINKED') if linked else value,
                  range=1, semitonesMode=0, formantPreserving=0)

def tremolo(rate=1, waveform=.005, depth=.85, phase=0):
    return effect('tremolo', rate=rate, waveform=waveform, depth=depth,
                  width=.5, phase=phase, quantize=1, triplet=0)

def phaser(period=2, feedback=.3, stages=7):
    return effect('phaser', lfo_period=period, feedback=feedback, stages=stages,
                  range=1, depth=.75, triplet=0, stereo=1)

def flanger(speed=8, regen=.3):
    return effect('flanger', speed=speed, width=6, manual=6.5, regen=regen, mix=1, triplet=0)

def reverb(decay=.55):
    return effect('reverb', decay=decay, bandwidth=.8, damping=.5, send_amount=(.5,'LINKED'))

def filt(hpf=13, lpf=18000):
    return effect('filter', hpf=hpf, lpf=lpf, q=.707106781)

# Single-mode standard Beat FX order. RMX expansion and Sound Color FX excluded.
CATALOG = [
 ('DELAY', [echo(.5,0)], 'Single forward repeat; no swing or separate left/right delay ratio.'),
 ('ECHO', [echo(.5,.6)], 'Forward feedback delay; native feedback and tail behavior.'),
 ('SPIRAL', [echo(.75,.7),phaser(4,.45)], 'Modulated echo tail; no pitch-changing feedback buffer.'),
 ('REVERB', [reverb()], 'Native reverb; different room and decay model.'),
 ('REV DELAY', [echo(.75,.4),tremolo(1,1,.8)], 'Smoothly gated forward repeats; does not reverse audio.'),
 ('MT DELAY', [echo(.25,.4),echo(.75,.3)], 'Two serial delay taps; no independently mixed multi-tap pattern.'),
 ('PITCH ECHO', [echo(.5,.5),pitch(.25)], 'Echo followed by a fixed upward pitch shift; pitch is not fed back.'),
 ('TRANS', [tremolo()], 'Tempo-locked amplitude chopping; native gate envelope.'),
 ('PAN', [effect('autopan',period=1,smoothing=.25,width=1)], 'Tempo-derived stereo panning; native phase and smoothing.'),
 ('FILTER', [tremolo(1,.5,.35),filt(180,5000)], 'Rhythmic amplitude modulation through a fixed band filter; no cutoff LFO.'),
 ('FLANGER', [flanger()], 'Native tempo-synchronized flanger; different delay and feedback model.'),
 ('PHASER', [phaser()], 'Native stereo phaser; different stage and sweep model.'),
 ('SLIP ROLL', [echo(.25,.85),tremolo(2,.005,.45)], 'Gated repeating live delay; no frozen slip buffer or transport slip.'),
 ('ROLL', [echo(.5,.9)], 'Long repeating live delay; continues receiving audio instead of freezing a loop.'),
 ('REV ROLL', [echo(.5,.88),tremolo(2,1,.9)], 'Smoothly gated repeat; no reversed or frozen loop.'),
 ('ROBOT', [effect('bitcrusher',bit_depth=6,downsample=.08),pitch(-.25)], 'Crushed, down-pitched robotic coloration; no vocoder or robot oscillator.'),
 ('PITCH', [pitch(0,True)], 'Native pitch shifter; Pitch parameter or Super knob sweeps its bipolar range.'),
 ('ENIGMA JET', [flanger(16,.65),phaser(4,.5,12)], 'Slow combined flanger/phaser sweep; no endlessly rising jet illusion.'),
 ('MOBIUS SAW', [phaser(4,.6,10),tremolo(.25,1,.5)], 'Resonant phaser with sinusoidal pulsing; no saw oscillator or infinite Shepard-tone rise.'),
 ('MOBIUS TRI', [phaser(4,.55,8),tremolo(.25,.5,.5)], 'Softer phaser with shaped pulsing; no triangle oscillator or infinite Shepard-tone rise.'),
 ('LOW CUT ECHO', [echo(.5,.65),filt(450)], 'High-pass-filtered echo output; filter is outside the feedback loop.'),
 ('PING PONG', [echo(.5,.6,1)], 'Alternating stereo feedback delay; native pan law.'),
 ('HELIX', [echo(.125,.85),phaser(1,.55,10)], 'Short resonant modulated repeats; no captured-buffer helix operation.'),
 ('VINYL BRAKE', [pitch(-.5,True),filt(13,3500)], 'Manual pitch sweep with dark filtering; does not slow or stop deck transport.'),
 ('STRETCH', [echo(1,.7),pitch(-.1666667),reverb(.7)], 'Long pitched repeats with reverb; does not time-stretch a captured buffer.'),
]

def render(name, effects, note):
    root=ET.Element('EffectChain')
    def add(parent,key,value):
        ET.SubElement(parent,key).text=str(value)
    add(root,'Name','[RB7] '+name)
    add(root,'Description','Native approximation: '+note)
    add(root,'MixMode','DRY/WET')
    add(root,'SuperParameterValue',.5)
    nodes=ET.SubElement(root,'Effects')
    for backend,parameters in effects:
        node=ET.SubElement(nodes,'Effect')
        add(node,'MetaParameterValue',.5)
        add(node,'Id','org.mixxx.effects.'+backend)
        add(node,'BackendType','Built-In')
        params=ET.SubElement(node,'Parameters')
        for key,value in parameters.items():
            value,link=value if isinstance(value,tuple) else (value,'NONE')
            p=ET.SubElement(params,'Parameter')
            for k,v in [('Id',key),('Value',value),('LinkType',link),('LinkInversion',0),('Hidden',0)]:
                add(p,k,v)
    ET.indent(root,space=' ')
    return ET.tostring(root,encoding='utf-8',xml_declaration=True)+b'\n'

if __name__=='__main__':
    target=ROOT/'res/effects/rekordbox7'
    target.mkdir(parents=True,exist_ok=True)
    for index,(name,effects,note) in enumerate(CATALOG,1):
        (target/f'{index:02}_{name.replace(" ","_")}.xml').write_bytes(render(name,effects,note))
