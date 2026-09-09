# DDJ-400 Kiosk Mapping for Custom Bite DJ

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This guide describes [Custom Bite DJ](../README.md), an independent fork of
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), based on Mixxx.

The Pioneer DDJ-400 mapping has been significantly overhauled to optimize performance for a standalone, touchscreen-driven workflow, mirroring modern club setups.

## 1. Simultaneous Beat FX Super & Mix Control
In upstream Mixxx, adjusting both the effect parameter (`super`) and the wet/dry ratio (`mix`) required toggling Shift on the single Level/Depth knob, preventing simultaneous dual-parameter sweeps.

**Custom fork modification:**
- **Shift + Filter Knob** (on either Deck 1 or Deck 2): Dynamically controls the main Beat FX **SUPER** knob (`[EffectRack1_EffectUnit1], super1`) with full 14-bit resolution.
- **Level/Depth Knob**: Directly controls the Beat FX **MIX** knob (`[EffectRack1_EffectUnit1], mix`).
- **Two-Handed Live Sweeps**: A DJ can hold Shift with their thumb and simultaneously sweep the **SUPER** knob with one hand (using either Filter knob) and the **MIX** knob with the other hand (using the Level/Depth knob).
- **Normal Filter Operation**: When Shift is not held, the Filter knobs control each deck's respective QuickEffect Filter (`[QuickEffectRack1_[ChannelN]], super1`) as standard.

### Effect selection

Press **BEAT FX SELECT** to advance one entry in the configured Beat FX list.
Hold **either deck's SHIFT** and press that same button to go backward.
Button release does nothing. Navigation follows the native preset list,
including its configured order and boundary behavior; there is no six-entry cap.
`chain_selector` is a relative encoder: send +1 forward or -1 backward,
never a preset index.

The mapping accepts normal SELECT (`0x94`, note `0x63`) with tracked Shift
state, as well as the dedicated shifted SELECT (`0x94`, note `0x64`). Deck 1/2
Shift is note `0x3F` on status `0x90`/`0x91`. Both paths update
`[EffectRack1_EffectUnit1],chain_selector` once per press.

Run `node tests/controllers/test_ddj400_effect_select.cjs` for the MIDI-binding
regression checks. On hardware, select a middle effect, hold each deck's Shift
in turn and press SELECT: verify one step backward, no step on release, and
forward selection after releasing Shift. Check the native list boundary at the first
effect. Hardware validation of this fallback remains pending.

## 2. Configurable Pad FX

Settings → PAD FX selects Deck 1/2, Normal/Shift bank and one of eight pads.
The 1024×600 editor shows one assignment at a time with effect, supported beat
length, strength and hold behavior. Reset restores the selected slot only.
Defaults and resets are owned by the system; opening the editor is optional.
The skin contains only the editor placement, with no Pad FX presets or DSP logic.
Changes are saved and apply on the next press; held pads keep their snapshot.
Stop All Pad FX clears both decks, including latched effects and pending tails.

DDJ-400 notes `0x10–0x17` and `0x60–0x67` share physical pad identities;
channels 7/8 select Deck 1 and 9/10 Deck 2, with even channels selecting Shift.
Note-off releases the original assignment even if Shift changed while held.
Private native effect lanes leave the user's Beat FX rack and faders untouched.
The former effect-number lookup and delayed 20 ms effect swap are removed.

Defaults follow the reviewed PiFlex bank (a change from our old fixed mapping):

| Pad | Normal | Shift |
| --- | --- | --- |
| 1 | Roll 1/2 | Trans 1/2 |
| 2 | Sweep | Crush |
| 3 | Flanger 16 | Filter LFO 4 |
| 4 | Release Brake 3/4 | Release Backspin 4 |
| 5 | Echo 1/4 | MT Delay 1/8 approximation |
| 6 | Echo 1/2 | Dub Echo approximation |
| 7 | Reverb | Space approximation |
| 8 | Release Echo 1/2 | Release Echo 1 |

These are native Mixxx approximations, not proprietary Rekordbox DSP. Only
Release Echo can latch; roll/brake/backspin remain momentary. Echo-family beat
overrides are bounded to 1/8–2 beats. Strength 0 disables an assignment.
Physical DDJ-400 timing, audio and LED validation is still required for this
new implementation; the historical verification below predates it.

## 3. Instant Doubles & Sync Fixes
- **Double-Tap Clone**: Pushing the rotary Load encoder twice in rapid succession triggers a custom Instant Doubles script. The script clones the currently playing track to the opposite deck, perfectly matching the playhead position, loop state, and pitch.
- **Sync Fixes**: Modified the Sync button to reliably snap to the beatgrid even during active Pad FX usage, resolving upstream issues where effect manipulation disrupted phase alignment.

## 4. UI Navigation & Hardware Disabling
- **Tab Toggling**: Pressing the Browse rotary encoder while holding SHIFT natively toggles the UI layout tab (`[Tab],current`), allowing you to expand the library to full-screen from the controller.
- **Crossfader Hardware Neutralization**: The JS callback explicitly intercepts physical MIDI slider inputs and drops them if the BiteDJ `[BiteDJ],crossfader_enabled` setting is disabled. This physically disconnects the controller's crossfader to prevent accidental bumps from bleeding audio.

## 5. Bundled Mixxx Effect Chains
Custom Bite DJ bundles Mixxx effect-chain presets in `res/effects/chains/` for
familiar club-style workflows. These are community implementations and
approximations, not official Pioneer DJ/AlphaTheta effect code or a claim of
identical DSP. Hardware and product names describe compatibility or inspiration:
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

## Standard Beat FX and touch picker

The [Beat FX catalogue](BEAT_FX.md) defines the 25 Rekordbox 7 single-mode names,
original native approximations and their limitations. The two-column picker
shows Standard and Saved separately; SELECT traverses the underlying standard
then saved order, including the empty entry at the wrap boundary. New standard
presets select Off and activate every occupied slot together from the existing
Effect1 `enabled` control. Physical DDJ-400 verification remains a hardware check.

BEAT left/right now writes periods `[0.125, 0.25, 0.5, 1, 2, 4]` to the first
loaded Beats-typed parameter's `parameterN_beat_period` alias. Native code
converts Tremolo's rate and preserves Echo's period semantics. Values clamp to
the manifest range and the screen highlights the actual applied period.
