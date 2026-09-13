# Custom Bite DJ 0.0.8 — in development

An independent fork of Team Deckshark’s BiteDJ, based on Mixxx.
Integration target: `codex/v0.0.8`. Development commits are published; this is not a tagged release.

## Changes

- Training Off/Line/Boxes is one saved General setting. Line and Boxes replace
  both stacked scrolling waveforms with a four-beat phase aid, retain the compact
  bottom overview waves plus time, pitch and absolute bar position, and mask BPM in Play, Grid and
  the deck previews on every other page. Holding the numeric BPM temporarily
  reveals it. Library-specific Grid, Played and Clear actions now live in the
  balanced Library settings footer.

- Local Time card touchscreen taps now open the date/time editor. Agent guidance
  requires native touch sequences through the widget hierarchy for touch tests.

- Shift + jog edits the beat grid only with the right-panel Grid tab active.
  Other tabs search the track gently, accelerating with wheel speed.
- Short and Long Vinyl Brake use native scratch release ramps. CDJ mode
  immediately releases an active scratch.
- Beat Sync takes the pressed deck’s current tempo as the source for the other
  deck. Turning Sync off preserves both adjusted tempos and releases phase lock.
  Quantize alone no longer aligns decks when pressing Play.
- Both main decks start with Quantize on. Manual Off lasts for the session,
  including subsequent track loads.
- GUI/rendering workers use normal scheduling; audio and reader threads retain
  dedicated real-time priorities.
- Browser previews prefer exported Rekordbox overview data on a bounded
  background worker. Rekordbox deck loads prefer the valid exported waveform;
  native cache and analysis remain fallbacks. Late batch analysis cannot replace
  the exported deck waveform. RGB uses Rekordbox PWV4/PWV5 colors independently
  of the PWV6/PWV7 envelopes used by 3 Band. Preview cache reads cannot create,
  migrate or repair files on the audio drive. Returning to an unchanged Play layout reuses renderer buffers.

## Validation and artifacts

Combined audio validation used `0.0.8-codex-v0-0-8-controller-validation.4`.
It passed 1,074 native tests (13 existing disabled) and eight desktop E2E tests;
the combined recording/controller build also passed 57 removable-drive tests.
The user validated the touchscreen clock fix on the Pi.

On the Pi, two PEN 2 WAV decks played while 3,103 controller browse steps ran
with waveform Preview enabled and recording to the same USB drive. The
120-second digital output capture and 198.8-second saved take had no
mid-playback silent 10ms blocks; the measured window logged no reader-cache
misses or sound-device underruns. A subsequent 22.2-second PEN 1 recording
also finalized without silent blocks. The user heard no audio breaks. The
output was the JBL Bluetooth sink; this does not establish performance on
every drive or output route. A sound-device underrun occurred before the
measured window.

The earlier build reproduced multi-second USB read starvation. Read-ahead now
keeps the urgent two-chunk request first and extends prefetch to 32 chunks
(about 5.9 seconds at 44.1kHz) within the existing 80-chunk cache. A full request
queue yields to the reader and retries on the next callback.

A cold Browse-to-Play load showed the full Play page around 0.73–0.84 seconds
after Load, with one intermediate layout frame and no captured audio gap.
Transition latency remains a measured limitation, not an instantaneous change.

The left source row removes D1/D2 labels, widens the source name and shows a
tiny USB icon only for mounted removable media. Recording status uses a fixed slot beside the source name only on decks using
the recording USB. When neither deck uses it, the top-right dot appears. Stop
clears all dots; playback arrows beside titles are removed.

Build `0.0.8-codex-v0-0-8-controller-validation.7` adds USB-specific recording
indicators and passed 1,076 native tests (13 existing disabled), 57 removable-drive
tests, fast checks and eight desktop E2E tests. The user validated the final
layout on the Pi. USB recording status was verified moving from the top-right
fallback to the matching source on load, then clearing after Stop while
playback continued.

Development binaries carry their full branch/build suffix; preserve their
provenance rather than relabeling them as the integration version.

See the [changelog](CHANGELOG.md), [test layers](docs/TESTING.md),
[DDJ-400 mapping](docs/DDJ400_MAPPING.md), and
[build/deployment guide](docs/BUILD_AND_DEPLOY.md).
A temporary application run on an existing Pi does not validate a freshly flashed
OS image. Sustained audio checks must exercise real USB WAV playback on both
decks, waveform preview loading, controller browsing, and track transitions.

Versioned release notes remain at the repository root. Historical notes retain
their own versioned filenames; [0.0.7](RELEASE_0.0.7.md) remains available.
