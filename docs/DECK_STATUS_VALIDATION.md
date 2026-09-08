# Deck status and INFO validation

Branch: `codex/deck-status-stems-system-info`.
Later merge target: `codex/v0.0.7`. No merge or version bump performed.

## Implemented

- Per-deck source labels use cached removable-drive identity: configured USB
  slots, volume names, LOCAL, OFFLINE and an empty unloaded state.
- The waveform sidebar keeps key/loop information, adds a separate beat-jump
  size and backward/forward actions, and labels QNT/LOCK with active styling.
  Beat-jump size follows the native control/controller; this first UI does not
  include a touch size picker.
- ON AIR reserves space beside the track title. Read-only `[ChannelN],on_air`
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
- Existing settings indices 0..4 and the Overview FX/KEY tabs are preserved.

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
