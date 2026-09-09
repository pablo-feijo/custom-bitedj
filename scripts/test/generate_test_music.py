#!/usr/bin/env python3
import math
import struct
import wave
import os
from pathlib import Path

SAMPLE_RATE = 44100

def synthesize_track(filename, bpm=128, bars=32):
    beat_dur = 60.0 / bpm
    bar_dur = beat_dur * 4
    total_duration = bars * bar_dur
    total_samples = int(total_duration * SAMPLE_RATE)
    
    print(f"Synthesizing {filename}: {bpm} BPM, {bars} bars ({total_duration:.1f}s)...")
    
    samples_l = [0.0] * total_samples
    samples_r = [0.0] * total_samples
    
    def add_sample(idx, val_l, val_r):
        if 0 <= idx < total_samples:
            samples_l[idx] += val_l
            samples_r[idx] += val_r
            
    # Drum hits synthesis
    kick_samples = int(0.25 * SAMPLE_RATE)
    kick_wave = []
    for i in range(kick_samples):
        t = i / SAMPLE_RATE
        freq = 150.0 * math.exp(-t * 28.0) + 45.0
        amp = math.exp(-t * 12.0)
        kick_wave.append(math.sin(2.0 * math.pi * freq * t) * amp * 0.85)
        
    hat_samples = int(0.08 * SAMPLE_RATE)
    import random
    rng = random.Random(42)
    hat_wave = []
    for i in range(hat_samples):
        t = i / SAMPLE_RATE
        amp = math.exp(-t * 45.0)
        noise = (rng.random() * 2.0 - 1.0)
        hat_wave.append(noise * amp * 0.25)
        
    snare_samples = int(0.18 * SAMPLE_RATE)
    snare_wave = []
    for i in range(snare_samples):
        t = i / SAMPLE_RATE
        tone = math.sin(2.0 * math.pi * 180.0 * t) * math.exp(-t * 20.0)
        noise = (rng.random() * 2.0 - 1.0) * math.exp(-t * 15.0)
        snare_wave.append((tone * 0.4 + noise * 0.6) * 0.6)
        
    # Place drums across all bars
    for bar in range(bars):
        bar_start_sample = int(bar * bar_dur * SAMPLE_RATE)
        for beat in range(4):
            beat_start_sample = bar_start_sample + int(beat * beat_dur * SAMPLE_RATE)
            # Kick on every beat
            if bar < 28 or bar >= 4:
                for i, s in enumerate(kick_wave):
                    add_sample(beat_start_sample + i, s, s)
            # Snare on 2 and 4
            if (beat == 1 or beat == 3) and (bar >= 4 and bar < 28):
                for i, s in enumerate(snare_wave):
                    add_sample(beat_start_sample + i, s * 0.9, s * 0.9)
            # Offbeat hi-hat
            offbeat_start = beat_start_sample + int(beat_dur * 0.5 * SAMPLE_RATE)
            for i, s in enumerate(hat_wave):
                add_sample(offbeat_start + i, s * 0.7, s * 0.8)
                
    # Bassline notes (Frequencies for C minor: C2, Eb2, F2, G2, Bb2)
    notes = [65.41, 77.78, 87.31, 98.00, 116.54]
    pattern = [0, 0, 1, 0, 2, 0, 1, 3, 0, 0, 4, 3, 2, 1, 2, 0]
    sixteenth = beat_dur / 4.0
    
    for bar in range(bars):
        if bar < 4 or bar >= 28:
            continue
        bar_start_sample = int(bar * bar_dur * SAMPLE_RATE)
        for step in range(16):
            step_sample = bar_start_sample + int(step * sixteenth * SAMPLE_RATE)
            freq = notes[pattern[step]]
            note_len = int(sixteenth * 0.85 * SAMPLE_RATE)
            for i in range(note_len):
                t = i / SAMPLE_RATE
                env = math.exp(-t * 10.0)
                phase = (t * freq) % 1.0
                val = (2.0 * phase - 1.0) * 0.35 + (1.0 if phase < 0.5 else -1.0) * 0.15
                add_sample(step_sample + i, val * env, val * env)
                
    # Lead synth chords
    chord_freqs = [
        [130.81, 155.56, 196.00],
        [116.54, 146.83, 174.61],
        [103.83, 130.81, 155.56],
        [116.54, 146.83, 174.61],
    ]
    for bar in range(bars):
        bar_start_sample = int(bar * bar_dur * SAMPLE_RATE)
        chord = chord_freqs[bar % len(chord_freqs)]
        chord_len = int(bar_dur * 0.95 * SAMPLE_RATE)
        for i in range(chord_len):
            t = i / SAMPLE_RATE
            env = math.exp(-t * 0.8) * (1.0 - math.exp(-t * 20.0))
            val_l = 0.0
            val_r = 0.0
            for k, f in enumerate(chord):
                val_l += math.sin(2.0 * math.pi * f * t) * 0.12
                val_r += math.sin(2.0 * math.pi * (f * 1.003) * t) * 0.12
            add_sample(bar_start_sample + i, val_l * env, val_r * env)

    max_amp = max(max(abs(s) for s in samples_l), max(abs(s) for s in samples_r), 1.0)
    norm = 0.92 / max_amp
    
    with wave.open(str(filename), 'wb') as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        raw_bytes = bytearray()
        for i in range(total_samples):
            sl = int(max(-32767, min(32767, samples_l[i] * norm * 32767)))
            sr = int(max(-32767, min(32767, samples_r[i] * norm * 32767)))
            raw_bytes.extend(struct.pack('<hh', sl, sr))
        wav.writeframes(raw_bytes)
        
    print(f"Generated {filename} successfully ({os.path.getsize(filename)} bytes).")

if __name__ == '__main__':
    output_dir = Path(__file__).resolve().parents[2] / 'test-music'
    output_dir.mkdir(exist_ok=True)
    synthesize_track(output_dir / 'BiteDJ_Test_Groove_128BPM.wav', bpm=128, bars=32)
    synthesize_track(output_dir / 'BiteDJ_Test_Techno_124BPM.wav', bpm=124, bars=32)
