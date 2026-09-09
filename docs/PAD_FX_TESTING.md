# Pad FX: implementation and local validation

Branch `codex/rekordbox-padfx-display`; eventual integration `codex/v0.0.7`.
The feature inherits the completed first batch, not another task's checkout.

## Behavior

System settings own defaults, saved assignments and reset commands. The skin
only places the optional native editor under Settings. The DDJ-400 mapping uses
private native effect lanes instead of replacing the user's Beat FX slot.
See [mapping details](DDJ400_MAPPING.md) for the new defaults and MIDI banks.

At 1024×600 the editor shows eight selectors and one assignment. It hides only
this Settings page's deck footer, fills the freed area with taller controls,
and preserves 16px horizontal / 12px vertical outer margins and 12px card padding.
Other Settings tabs restore the footer. Dropdowns show six touch-scrollable
rows. Night/day colors follow the existing system preference.

## Checks

- Native settings validation, disk round trip, deck isolation and system reset
  work without constructing the editor.
- Native private-lane control-to-audio routing and Pad Echo buffer bounds,
  dry-path restoration and tail decay at 44.1/48 kHz and 32/256/1024 frames.
- Native editor bounds, minimum 44px targets, bounded dropdown, full-height card
  and daylight label color; related library, Rekordbox and touch regressions.
- JavaScript controller cases: held-pad snapshots, overlap ordering, Shift
  note-off, strength/timing bounds, latch, stop, shutdown, unavailable backend,
  and DDJ-400 normal/shadow note and LED routing.
- Live ARM64 GUI process: raw MIDI bytes through its real PortMidi dispatch,
  installed DDJ-400 XML/JavaScript, real effects engine and PulseAudio monitor.
  Sweep attenuates the 8 kHz test component; note-off restores dry output.
  Echo and Reverb produce decaying tails after stopping the source and releasing.

The live harness replaces only the hardware transport with an explicitly
preloaded test library: Docker has no ALSA sequencer. Its UDP input is bound to
127.0.0.1 inside the owned container, with no published MIDI port. It is not
installed in the product and is removed from the process environment on cleanup.
This does not qualify physical USB/controller timing or Pi audio hardware.

## Reproduce

Build and launch from this worktree, using a free instance/ports:

```bash
./scripts/build/docker-build.sh --platform linux/arm64
BITEDJ_TEST_INSTANCE=bitedj-next-gui \
BITEDJ_TEST_WEB_PORT=6082 BITEDJ_TEST_AUDIO_PORT=8002 BITEDJ_TEST_VNC_PORT=5902 \
./scripts/test/run-gui-test.sh
./tests/padfx/test-live-midi-audio.sh
node tests/padfx/test_padfx.cjs
```

The live script verifies container ownership, snapshots/restores only this
instance's settings, runs the calibration captures, then restarts its normal
demo tracks. It requires the existing ARM64 builder image, Python 3 and Docker.
It does not stop other branches' containers. Recordings, measurements and raw
MIDI logs are in `test-results/<instance>/padfx-audio/` and `pad-midi-live.log`.
The calibration uses steady 1 kHz/8 kHz tones so before/after comparisons do not
depend on changing musical content. Capture assertions reject silence.

Native tests are part of `mixxx-test`. After configuring `BUILD_TESTING=ON`, run:

```bash
QT_QPA_PLATFORM=offscreen ./build-linux/mixxx-test \
  --gtest_filter='PadFx*:PadEcho*:EffectSlotTest.*:LibraryColumnControlTest.*:RekordboxAnlzTest.*:RekordboxPageChainTest.*:ControllerLibraryColumnIDRegressionTest.*:TouchScrollFilterTest.*:PlayedTracksTest.*'
```

Run that ARM64 command inside the builder container, with this worktree mounted
at `/src`, as documented in the first-batch guide. Do not invoke the ARM64 binary
as a native macOS executable.

## Recorded ARM64 result

40 native tests pass, plus the JavaScript controller suite and two live MIDI
capture runs. In the repeatable-script run, Sweep reduced the 8 kHz component
by 10.79 dB; release RMS returned within 2.7% of the baseline (the capture check
allows 10%). Echo's tail dropped 49.5 dB and Reverb's 33.5 dB between the first
and second seconds. These are container captures, not hardware latency claims.
The initial manual run also passed; its release level matched within 0.001%.
