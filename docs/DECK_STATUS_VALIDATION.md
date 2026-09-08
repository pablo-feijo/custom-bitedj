# Deck status and INFO validation

Branch: `codex/deck-status-stems-system-info`.
Later merge target: `codex/v0.0.7`. No merge or version bump performed.

## Implemented

- Per-deck source labels use cached removable-drive identity: configured USB
  slots, volume names, LOCAL, OFFLINE and an empty unloaded state.
- The waveform sidebar keeps source/key/loop information and labels QNT/LOCK
  with active styling. The right panel now contains FX, KEY and JUMP tabs.
  JUMP offers per-deck size readouts, halve/double size buttons and backward/
  forward actions. The sidebar is 126px wide; the active-output badge reads ON at 26x16.
- The ON badge reserves space beside the track title. Read-only `[ChannelN],on_air`
  follows transport/passthrough, fader/crossfader/mute and an actual connected
  main output. Quiet passages do not flicker. Disconnecting the last main
  output clears it even if callbacks stop. External mixer audibility cannot be
  inferred; headphone-only routing does not light the badge.
- Read-only `[Master],main_output_connected` reflects actual main-output
  connections separately from the internal main buffer used for headphone mix.
- Settings -> INFO adds callback load, whole-system CPU usage, CPU/SoC
  temperature, local clock and main-output configuration/connection state.
  Pending audio edits are explicitly labeled. CPU/sensor reads run on a worker
  once per second, with missing/stale data shown as N/A.
- Existing settings indices 0..4 are preserved. JUMP is appended after FX/KEY
  per the user's layout refinement; PADS and CFX remain absent.

## Checks

- ARM64 build through `./docker-build.sh --platform linux/arm64`: passed.
- Final native test selection: **14 passed** across SystemSettingsTest,
  SystemTelemetryTest, TrackSourceWidgetTest, EngineBufferTest.OnAir* and
  EngineMixerTest. Covers path boundaries/nested mounts/unplug classification,
  label load/replacement/unload, CPU counter reset/guest accounting, invalid
  temperature input, transport/fader/mute/crossfader/headphone-only routing and
  output disconnection, plus existing mixer output golden tests.
- XML parsing and `git diff --check`: passed.
- 1024x600 visual inspection: PLAY, INFO, day/night styling, source labels,
  paused/playing badges and title elision checked.
- Paused beat jump at 128 BPM: four beats moved remaining time from 1:59.99
  to 1:58.12 (1.87 seconds), without starting playback.
- PulseAudio capture: paused RMS 0; playing RMS 3719.56, peak 18429 (16-bit
  PCM). Confirms actual audio reached the local output during native-button
  playback, not merely a changing transport indicator.

## Local deployment

The primary `bitedj-gui-test-instance` now mounts this checkout's `dist-linux`,
test music and test config. VNC is available at http://localhost:6080/vnc.html
and the audio page at http://localhost:8000/. HTTP checks returned 200 for VNC,
its rfb.js module and the audio page.

The previous container was mounted to `custom-bitedj-work`; it is retained,
stopped, as `bitedj-previous-checkout-gui-instance`. The temporary isolated
feature test container is also stopped. No files in the other checkout were
replaced with this build.

The documented `test-gui-automated.sh` is absent in this checkout. The available
`test-gui-fx.sh` was exercised via a temporary harness: container/port overrides
for isolation, repeatable screenshot capture and bounded stream reads (the
original curl/head pipeline exits with a broken-pipe error under pipefail).
Its byte-comparison check is a smoke check, not proof of effect DSP correctness;
the PCM capture and native mixer tests provide the relevant audio evidence.

## Remaining scope

Real Pi temperature/USB topology, minimum-resolution behavior below 1024x600,
physical controller input and long-duration hardware performance have not been
validated. The container exposes no Pi thermal sensor and correctly shows N/A.
Native stem mixing is not implemented; its dependency assessment and required
hardware proof are recorded in STEM_FEASIBILITY.md.

## Layout refinement after review

- The badge now reads ON. Sidebar captions were simplified to key and loop
  values; source, metadata, state buttons and transport have an 8px group gap
  and are vertically centered in each waveform lane.
- FX removes the duplicate MIX meter, uses 44px selector/routing/activation
  controls and keeps its native beat grid intact. Roll was visually checked
  with that grid visible and both MIX/SUPER knobs fitting above the deck strips.
- KEY and JUMP share 148px deck blocks, 28px headers, 44px control rows and
  consistent 8px gaps. KEY reset now occupies a full-width second row.
- JUMP was verified at both 2 and 4 beats; halve/double and forward/back operate
  on the selected deck only. KEY +2 changed Cm to Dm and Reset restored Cm.
- Day/night layouts and the shortened active-output badge were inspected in
  the updated primary VNC instance. XML parsing and diff checks passed.

### Sidebar typography follow-up

Restored 9px KEY/LOOP captions above 12px values in two aligned columns.
Deck/source badges now use 11px type, separate backgrounds and a 6px gap.
The track-strip key and CUE badges use 10px type at 16px height, with an 8px
trailing gap after CUE. Verified in the primary 1024x600 VNC view; skin XML
parsing and whitespace checks passed. No native code changed.

### Compact top menu and playback label

Reduced the main bar from 60px to 48px, trimming vertical button margins to
preserve label clearance. Removed the orange playback arrows around the deck
number. Verified the 1024x600 overview during playback: deck labels stay stable
and the ON badge still activates. Updated VNC; XML parsing and diff checks pass.

Top-menu button follow-up: reduced label type from 17px to 15px and increased
vertical margins from 4px to 7px, shortening the buttons within the 48px bar.

Further menu refinement: 40px total bar height, 14px labels, 26px button
height and explicit 10px gaps between buttons. Visually checked at 1024x600
in the updated VNC instance; XML parsing and whitespace checks passed.
