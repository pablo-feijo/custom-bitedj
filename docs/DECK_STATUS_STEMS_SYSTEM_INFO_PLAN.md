# Deck status, stems and system information

Status: first implementation complete for source labels, beat jump, ON AIR,
active quantize/keylock styling and Settings -> INFO. Stems remain a separate
engine integration; see STEM_FEASIBILITY.md. Targeted tests pass; real Pi
sensor/USB and stem performance validation remain hardware follow-ups.
Branch: `codex/deck-status-stems-system-info`, based on `v0.0.6`.
Later merge target: `codex/v0.0.7` (user-directed). No merge or release version
bump is part of this feature implementation.
Reference inspected: `ntamas94/pioneered-by-ntamas` at
`db9b669f1cb2fa24f9316cfde855e1e2c6643449` (2026-09-08).

## Requested result

Bring the reference's clear source/key/beat-jump information, colored stem
controls, ON AIR badge, active deck states and system readouts into BiteDJ.
Adapt them to the 1024x600 touchscreen and BiteDJ's existing design.

## Findings that affect scope

- The reference's waveform `USB1` text is hardcoded. BiteDJ should show the
  loaded track's actual source using its existing removable-drive enumeration
  and configured physical USB slot mapping.
- BiteDJ already displays key, BPM, pitch range/adjustment, loop size, quantize
  and keylock. Improve grouping and state visibility instead of duplicating them.
- Native `beatjump_size`, halve/double and forward/back controls already exist
  in `src/engine/controls/loopingcontrol.cpp`.
- Reference ON AIR binds to a custom `[Pioneered],onairN` control. Copying XML
  alone would not provide a working indicator. BiteDJ needs an owned state
  derived from its mixer routing and playback behavior.
- The reference's three colored buttons request pre-rendered audio files through
  a controller/daemon bridge. They do not independently mute stems in the loaded
  track. Its separate four-stem mixer strip targets a stem-capable engine.
  BiteDJ currently has no corresponding stem engine/widget controls in the
  inspected engine, mixer, track or widget code.
- LOAD means audio callback utilization (`[App],audio_latency_usage`), distinct
  from system CPU utilization. Their CPU/temperature readings arrive through
  a Pi MIDI bridge. BiteDJ can expose these directly through native code.
- The reference's OUT button is documented as unconnected. It is not evidence
  of output routing/status. If we show output information, use BiteDJ's actual
  configured audio device and main-output state.
- Settings -> System already reserves space for USB drives and power/display
  controls. A separate INFO tab is the proposed home for diagnostics.

## Proposed layout and behavior

### 1. Source, key and beat jump

Use the existing 126px waveform information column as the starting point.
Arrange a compact source label and labeled musical key/loop readouts.
Following user review, beat jump lives in the right panel beside FX and KEY.
Preserve access to loop size, quantize, keylock, play and cue. Exact geometry
must be settled in a 1024x600 mockup before editing skin XML; the current column
already contains these controls and cannot simply absorb extra rows.

Source states: USB slot/name for mapped removable storage, a short volume name
for other removable storage, LOCAL for internal tracks, and an empty state when
no track is loaded. Handle long names, multiple partitions, unplugging and
track replacement. Use existing device identity rules rather than enumeration
order to assign USB numbers. Keep eject in the existing System page initially.

Show beat jump independently of loop length in a dedicated JUMP tab, with
per-deck halve/double size buttons and backward/forward actions using native controls.
Reserve at least 44px touch targets for new interactive controls.

### 2. ON AIR and active deck states

Place a compact red ON badge (main-output activity) beside each track title, with a corresponding
small waveform indication only if the mockup leaves sufficient space.
Reserve its space so titles do not shift when it changes.

Define ON AIR as a deck contributing through the application's main mix path.
Validate play/end-of-track, channel fader, crossfader position and assignment,
main routing and mute behavior; headphone-only cue must not light it. Confirm
how passthrough and external mixer routing should behave before implementation.
It cannot assert that external speakers are physically producing sound.
Prefer routing state over instantaneous amplitude so quiet passages do not
flicker. Review existing engine audibility logic for reuse.

Make QUANTIZE, KEY LOCK, SYNC/tempo leader (where applicable), loop state, BPM,
pitch range and pitch adjustment legible with consistent active/inactive styling.
Bind indicators to existing controls so controller changes remain synchronized.

### 3. Settings -> INFO

Add an INFO tab containing audio LOAD %, system CPU %, SoC temperature and
local clock. Include real main-output device/status if the current audio
settings backend can provide it reliably. Clearly distinguish audio load from
CPU usage. Use N/A for unsupported readings and a stale/unavailable state for
failed readings rather than displaying a misleading zero.

Use the native audio-load control and Time widget. Add lightweight native
Linux telemetry for CPU and temperature, sampled about once per second away
from the audio callback; keep OS reads off the GUI thread. Avoid a MIDI bridge
or desktop overlay dependency. Verify the thermal sensor identity on the Pi.

Append the new settings page without changing persisted indices of existing
pages. Check six-tab sizing, day/night mode and the supported minimum layout.
An optional compact status display during playback is a later layout choice,
not required to deliver the requested settings view.

### 4. Stems: feasibility gate before UI implementation

Preferred product behavior: independently enable/mute parts while preserving
the deck's playback position. The reference's file-replacement behavior does
not establish that capability in BiteDJ.

First assess a scoped stem-engine backport versus an engine upgrade, reading
`docs/DIFFS_FROM_BASE.md` before either. Identify decoder/manifest, buffering,
mixing, controls and widget dependencies; review licenses before reusing code.
Use pre-separated files for the initial feasibility test; live AI separation
is a separate feature and is not assumed in this plan.

Prove two-deck playback on the target Pi, acceptable callback load, independent
mute, position continuity, seeking and looping. Map names from real stem
metadata; a three-button DRUMS/VOCAL/INST presentation needs an explicit rule
for combining bass and other instruments when files contain four parts.

Only after that proof, add a compact colored strip adjacent to each waveform,
with textual state as well as color and a settings visibility option. Show
controls only for supported content. Keep ordinary stereo playback intact.
If engine work is too large, deliver the other features independently and
revisit the stem scope; do not ship decorative controls that cannot work.

## Delivery order and acceptance

1. Review this proposal, then prepare measured 1024x600 layout mockups for deck
   status, waveform information and the INFO page.
2. Implement source identity and beat-jump display/actions.
3. Implement ON AIR and improve existing active-state indicators.
4. Implement INFO telemetry and settings persistence.
5. Complete stem feasibility and review its resulting scope before integration.

Likely touchpoints: `res/skins/BiteDJ/{deck,waveform,settings,skin}.xml`,
`res/skins/BiteDJ/style.qss`, `src/preferences/systemsettings.*`, an appropriate
native telemetry/source-label component, and mixer/player controls for ON AIR.
Keep Overview FX/KEY/JUMP as specified in AGENTS.md; do not restore PADS
or CFX tabs or alter the native Beats parameter grid.

For implementation, add focused tests for source classification, ON AIR state
transitions and telemetry unavailable/stale behavior. Check beat jump while
playing/paused, looping, with quantize on/off and from controller input. Run
`./test-gui-automated.sh`, capture the container with scrot and inspect spacing,
long titles, empty decks and day/night modes at 1024x600. Verify CPU/temperature,
USB identity and sustained two-deck audio on actual Pi hardware. Stem audio
tests must prove independent parts and continuity, not merely button state.

## Reference sources

- https://github.com/ntamas94/pioneered-by-ntamas/tree/main/pi-setup
- https://github.com/ntamas94/pioneered-by-ntamas/blob/main/docs/the-skin.md
- https://github.com/ntamas94/pioneered-by-ntamas/blob/main/skin/Pioneered_by_ntamas/waveform.xml
- https://github.com/ntamas94/pioneered-by-ntamas/blob/main/skin/Pioneered_by_ntamas/deck.xml
- https://github.com/ntamas94/pioneered-by-ntamas/blob/main/skin/Pioneered_by_ntamas/topbar.xml
