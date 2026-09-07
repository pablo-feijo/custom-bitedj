# DDJ-400 Kiosk Mapping for BiteDJ

The Pioneer DDJ-400 mapping has been significantly overhauled to optimize performance for a standalone, touchscreen-driven workflow, mirroring modern club setups.

## 1. Beat FX and Release FX Re-routing
In upstream Mixxx, the Level/Depth knob on the DDJ-400 only adjusts the master dry/wet `mix` parameter. The user must hold SHIFT to adjust the specific effect's `meta` parameter (e.g., Echo feedback, Reverb decay, Filter frequency).

**BiteDJ Modification:**
The Level/Depth knob has been patched in JS to adjust **both** the `mix` and `meta` values simultaneously when turned.
- This creates an instant "Release FX" style sweep.
- Applying an Echo or Reverb via the Blue ON/OFF button and turning the knob instantly intensifies the trail and pushes the dry/wet ratio without needing SHIFT.

## 2. Pad FX 1 Integration
The DDJ-400 has a dedicated hardware mode for "Pad FX 1" which was largely underutilized or buggy in upstream Mixxx.
BiteDJ maps MIDI notes `0x10-0x17` (and their `0x60-0x67` shadows) directly to high-impact momentary effects.

**How it Works:**
- Pressing and holding any pad instantly loads a pre-configured effect profile.
- The `mix` and `meta` values jump to optimal presets instantly, bypassing the physical Level/Depth knob.
- A `20ms` initialization timer prevents audio dropouts during the rapid effect-swapping phase inside the Mixxx engine.
- Releasing the pad restores the previous active state, allowing seamless punch-in effects without altering the Beat FX chain.

### Pad FX Profiles
The following profiles are currently hardcoded for Pad FX1 mode:
* **Pad 1**: Echo (1/2 beat)
* **Pad 2**: Echo (1 beat)
* **Pad 3**: Echo (2 beats)
* **Pad 4**: Echo (4 beats)
* **Pad 5**: Flanger (Fast)
* **Pad 6**: Flanger (Medium)
* **Pad 7**: Flanger (Slow)
* **Pad 8**: Reverb

## 3. Instant Doubles & Sync Fixes
- **Double-Tap Clone**: Pushing the rotary Load encoder twice in rapid succession triggers a custom Instant Doubles script. The script clones the currently playing track to the opposite deck, perfectly matching the playhead position, loop state, and pitch.
- **Sync Fixes**: Modified the Sync button to reliably snap to the beatgrid even during active Pad FX usage, resolving upstream issues where effect manipulation disrupted phase alignment.

## 4. UI Navigation & Hardware Disabling
- **Tab Toggling**: Pressing the Browse rotary encoder while holding SHIFT natively toggles the UI layout tab (`[Tab],current`), allowing you to expand the library to full-screen from the controller.
- **Crossfader Hardware Neutralization**: The JS callback explicitly intercepts physical MIDI slider inputs and drops them if the BiteDJ `[BiteDJ],crossfader_enabled` setting is disabled. This physically disconnects the controller's crossfader to prevent accidental bumps from bleeding audio.

## 5. Bundled Pioneer Effect Chains
BiteDJ bundles the complete standard Pioneer DJ club mixer effect chains in `res/effects/chains/`:
- **Color FX (QuickEffect Racks)**:
  - `C_Crush`: Bitcrusher downsampling curve tailored for filter sweeps.
  - `C_Filter`: Standard club bipolar resonant low-pass / high-pass sweep.
  - `C_Noise`: Filtered white noise generator with high/low cut tracking.
- **Beat FX (Main Effect Units)**:
  - `DELAY`: Pioneer style tempo-synced delay.
  - `ECHO`: Classic quantified echo with feedback loop.
  - `FILTER`: LFO-modulated tempo-synced filter sweep.
  - `FLANGER`: Jet engine flanger sweeping through frequencies.
  - `PAN`: Automatic stereo field panning.
  - `PHASER`: Multi-stage phase shifter.
  - `PINGPONG`: Alternating stereo channel ping pong delay.
  - `REVERB`: High-density room/hall reverberation.
  - `TRANS`: Rhythmic transformer gating / slicer.

## 6. Hardware Verification

The DDJ-400 mapping has been verified end-to-end on physical hardware connected to BiteDJ:
- **Transport & Jog Wheels**: Play, Cue, Scratching, and Pitch Bend tracked with zero latency.
- **Mixer & EQ**: Channel faders, 3-band EQ, Trim, Master output, and headphone cueing.
- **Beat FX**: Level/Depth simultaneous mix/meta sweep, ON/OFF toggle, Beat FX channel assignment.
- **Pad FX**: Instant punch-in profiles on Pads 1–8 with automatic clean restoration on release.
- **In-Skin Device Picker**: Discovered and enabled under **Settings -> Devices**.
