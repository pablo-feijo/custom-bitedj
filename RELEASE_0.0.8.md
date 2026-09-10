# Custom Bite DJ 0.0.8 — in development

An independent fork of Team Deckshark’s BiteDJ, based on Mixxx.
Integration target: `codex/v0.0.8`. This version has not been published.

## Changes

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

Validated development build: `0.0.8-codex-v0-0-8-controller-validation.2`.
Fast checks, 1,061 native tests, 57 removable-drive tests and eight desktop E2E
tests passed (13 existing native tests remain disabled). The user reviewed this
build on the Pi with DDJ-400 and the Techno USB playlist; exported RGB previews
were verified. These results do not establish uninterrupted audio on every drive.

The earlier PEN 2 two-WAV browsing stress run produced reader starvation and
long digital-output gaps despite zero callback-underrun reports. A controlled
60-second PEN 1 run had no mid-playback silence, but PEN 2 cold-load stress must
still be repeated on this build. Transition latency and combined USB recording
stress also remain open. Keep audio capture and reader-cache diagnostics in
future performance checks.

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
