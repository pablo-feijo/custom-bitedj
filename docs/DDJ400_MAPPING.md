# DDJ-400 Kiosk Mapping for BiteDJ

The Pioneer DDJ-400 mapping has been significantly overhauled to optimize performance for a standalone, touchscreen-driven workflow, mirroring modern club setups.

## 1. Simultaneous Beat FX Super & Mix Control
In upstream Mixxx, adjusting both the effect parameter (`super`) and the wet/dry ratio (`mix`) required toggling Shift on the single Level/Depth knob, preventing simultaneous dual-parameter sweeps.

**BiteDJ Modification:**
- **Shift + Filter Knob** (on either Deck 1 or Deck 2): Dynamically controls the main Beat FX **SUPER** knob (`[EffectRack1_EffectUnit1], super1`) with full 14-bit resolution.
- **Level/Depth Knob**: Directly controls the Beat FX **MIX** knob (`[EffectRack1_EffectUnit1], mix`).
- **Two-Handed Live Sweeps**: A DJ can hold Shift with their thumb and simultaneously sweep the **SUPER** knob with one hand (using either Filter knob) and the **MIX** knob with the other hand (using the Level/Depth knob).
- **Normal Filter Operation**: When Shift is not held, the Filter knobs control each deck's respective QuickEffect Filter (`[QuickEffectRack1_[ChannelN]], super1`) as standard.

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

## Controller pad drawer

The Overview cue drawer follows the DDJ-400 mode selected on either deck.
Hot Cue restores the existing Hot Cues/Memory controls; Pad FX, Beat Jump and
Beat Loop replace the pad grid with a **read-only controller legend**. Use the
physical pads to perform the displayed actions. X still closes the drawer;
selecting a controller mode opens that deck again. Mode button release does
not close it. Other modes close only their own deck's visible drawer.

Runtime controls (not saved): `[PadFX],dN_mode` = 0 Hot Cue, 1 Pad FX,
2 Beat Jump, 3 Beat Loop, 4 Memory; `dN_shift` = 0/1; `dN_jump_bank` = 0/1/2 for
1/16, 1, 16 multipliers. The existing mapping shares jump sizes across decks.
The header stays 30px high; the legend uses the existing four-column/two-row
area, with a 44px minimum row height. Verified at 1024×600: header y=450–481, X center (1000,467);
legend columns centered at x=128/384/640/896, rows at y=510/570.
Each legend cell is 252×56px. The waveform area stays at y=39–449.
See [controller drawer screenshots](UI_SCREENSHOTS.md#controller-pad-drawer).

The legend follows saved Normal/Shift assignments, timing overrides, strength
and toggle settings. Beat Loop shows four held rolls and four toggle loops;
holding Shift shows its mapped shifted Pad FX assignments. Beat Jump's Shift
bank shows size ÷16 / ×16 on pads 7/8. Modes are independent per deck.

Mode selection uses MIDI status `0x90`/`0x91`: Hot Cue `0x1B`, Beat Loop
`0x6D`, Beat Jump `0x20`, Pad FX1 `0x1E`. Keyboard `0x69`, Pad FX2 `0x6B`,
Sampler `0x22`, and Key Shift `0x6F` dismiss this legend; these secondary modes
are not newly implemented by this display change.
Source: [AlphaTheta DDJ-400 MIDI message list, page 3](https://downloads.support.alphatheta.com/software_info/dj-controllers/DDJ-400/DDJ-400_MIDI_Message_List_E1.pdf).

Run `node tests/padfx/test_controller_pad_display.cjs` and
`node tests/padfx/test_padfx.cjs` for the mapping regressions.

Touch drawer navigation: tap the header at (530,467) to cycle Hot Cues → Memory
→ Beat Jump → Pad FX → Beat Loop → Hot Cues. `[PadFX],dN_cycle` is a momentary
command; release does not advance. Controller and touch selection share
`dN_mode`, while the two decks keep independent selections. Touch navigation
works without a connected controller. The performance legend remains read-only;
this change adds header navigation, not new touch performance actions.
