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

## Controller pad drawer

The Overview cue drawer follows the DDJ-400 mode selected on either deck.
Hot Cue restores the existing Hot Cues/Memory controls; Pad FX, Beat Jump and
Beat Loop replace the pad grid with a **read-only controller legend**. Use the
physical pads to perform the displayed actions. X still closes the drawer;
selecting a controller mode opens that deck again. Mode button release does
not close it. Other modes close only their own deck's visible drawer.

After selecting a pad mode (including Shift + Pad Mode), press **Shift three
times within 1.2 seconds** to close the drawer and restore the bottom waveform
previews. Either Shift button works, including alternating sides. Mode selection
resets the count; holding Shift or releasing it does not add presses. The gesture
keeps each deck's selected pad mode and normal Shift behavior intact.

Runtime controls (not saved): `[PadFX],dN_mode` = 0 Hot Cue, 1 Pad FX,
2 Beat Jump, 3 Beat Loop, 4 Memory; `dN_shift` = 0/1; `dN_jump_bank` = 0/1/2 for
1/16, 1, 16 multipliers. The existing mapping shares jump sizes across decks.

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


## Touch drawer mode navigation

The drawer follows controller mode selection independently for Deck 1 and Deck 2.
Touch **Previous** (`[PadFX],dN_previous`) and **Next** (`[PadFX],dN_cycle`) step
through Hot Cues → Memory → Beat Jump → Pad FX → Beat Loop and wrap in either
direction. Both are momentary: only the press advances; release does not.
Numeric IDs remain 0, 4, 2, 1, 3 in that display order. Controller selection
updates the same `dN_mode` state, so touch navigation continues from that mode.
Performance pages are assignment legends; they do not trigger pads or change
hardware pad routing. Hot Cue/Memory pages retain their existing touch actions.
See [drawer geometry and screenshots](UI_SCREENSHOTS.md#controller-pad-drawer).

## Automated mapping coverage

Run `node tests/controllers/test_ddj400_mappings.cjs` for the complete MIDI
binding regression suite, or `python3 scripts/test/run-tests.py fast` to include
it with the existing Pad FX, drawer and effect-selection tests. CI runs the fast
suite before native compilation. Node.js 22+ is required.

The harness loads the shipped mapping and Pad FX scripts and dispatches events
using the actual XML status/note/group bindings. A final audit requires unique
inputs, resolvable callbacks, behavioral execution of every script binding and
an explicit target/options assertion for every native binding. New bindings
therefore require coverage. This is a binding/behavior audit, not a claim of
100% JavaScript branch coverage.

| Area | Regression coverage |
| --- | --- |
| Filter and mixer | Independent 14-bit filters, either Shift to Super, simultaneous Mix/Super/Sync while Pad FX is held, crossfader disable/re-enable |
| Beat FX | Relative selection, routing to either/both decks, enable LEDs, panic, first loaded Beats parameter, all buckets, endpoints, off-grid values and releases |
| Browse and load | Play waveform zoom, signed library scrolling, Shift tab switch, per-deck loading, Instant Doubles before/at 500 ms, source play/sync state |
| Deck controls | Sync short/long presses, tempo ranges/sliders, live Vinyl/CDJ setting, jog bend/scratch/grid alignment, loop adjustment, quick jumps and quantize |
| Performance pads | Every Pad FX pad/bank/deck, real and zero-velocity note-offs, duplicate presses, Shift changes while held, LED identity, Beat Jump banks/limits |
| Drawer | All supported/unsupported mode notes, deck isolation, Shift state and triple-Shift waveform return; timeout/held/release cases in the existing drawer suite |
| Samplers and lifecycle | All 16 load/play/stop/eject pads, paired Shift LEDs, startup query, track/VU feedback, pending/active loop blink and Pad FX cleanup |
| Native XML mappings | Transport, mixer/EQ, headphones, Hot Cues and Beat Loop target, deck, MIDI-resolution and soft-takeover contracts |

Engine controls, time, timers and MIDI output are simulated. Native control
semantics, DSP/audio, device firmware messages and physical LED behavior still
need native tests or a connected DDJ-400; this suite does not emulate them.

## Jog alignment, release and service smoothing

Shift + jog translates the beatgrid through `beats_translate_move` (signed jog
steps), without seeking or enabling scratch. This takes priority over loop-point
adjustment. Dedicated shifted rotation/touch notes work even without a Shift
press message; pressing Shift during an active scratch releases scratch immediately.
Shift + Browse rotation zooms from any page through the existing linked waveform
zoom path; Shift + Browse press continues to toggle Play/Browse.

Jog release uses `scratchDisable(deck, false)`, restoring `play` only if that
scratch began on a playing deck. Paused decks stay paused. Release is handled even
if loop adjustment was enabled during touch.

Service Preferences → Decks → Deck options → Jog-wheel smoothing saves
`[Controls] JogWheelFilterLength`: default 6, minimum 1, maximum 64. Apply updates
all decks on their next audio callback; Cancel leaves the running filter alone,
and Reset restores 6 when applied. This controls pitch-bend smoothing, not the
controller scratch tick resolution. Saved out-of-range integers are clamped.

SELECT / Shift + SELECT retain next/previous preset selection. BEAT left/right
steps the focused slot's first Beats-typed parameter shorter/longer. ON/OFF toggles
the focused slot (Effect1 when no slot is focused), and its LEDs follow focus.
Either deck's Shift + ON/OFF disables all three slots, including when the controller
sends the normal ON/OFF note. The dedicated shifted note retains the same action.

### Active-loop jog resizing

While a loop is active, normal jog rotation halves its length counterclockwise
and doubles it clockwise. One resize requires 32 cumulative jog ticks (about
16° at the mapping's 720 ticks/revolution). Reversing direction discards the
partial turn; releasing touch, changing loop state or using Shift clears it.
Each deck accumulates independently. Native `loop_scale` keeps the start point
and enforces the minimum audible length and track-end limit. Playing decks keep
playing; paused decks remain paused. Leaving the loop restores jog pitch bend
and Vinyl-mode scratching. Shift + jog always takes priority for grid alignment.
Explicit Shift + LOOP IN/OUT boundary-edit modes retain fine adjustment after
releasing Shift; toggle the mode off to return to automatic half/double resizing.
