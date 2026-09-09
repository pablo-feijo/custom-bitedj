# Changelog

Notable changes to Custom BiteDJ are recorded here. Versions use Semantic
Versioning; commit messages follow Conventional Commits.

## [0.0.7] — Unreleased

### Added

- Deck source indicators for USB volumes, local files and removed media, plus a
  compact ON badge tied to playback and main-output routing.
- A JUMP panel with per-deck beat-jump controls and linked waveform zoom/reset.
- Independent elapsed/remaining time selection and scrolling long track titles.
- Settings → INFO with audio load, CPU usage, temperature, clock and output status.
- Deck drop-target highlighting with matching drop geometry and Escape cancellation.
- Sparse minute rulers and an optional blue/amber/cream waveform palette.

### Changed

- Reduced top-menu height and button size, with wider gaps between buttons.
- Simplified deck metadata and aligned FX, KEY and JUMP controls.
- Compacted the Browse table, headers and breadcrumb to leave more room for tracks.
- Sized overview waveforms at 38px and minute rulers at 12px.

### Fixed

- Clipped lower waveforms and stacked rendering outside the image boundary.
- 3 BAND preview colors, band order and baseline now match the main waveform's
  band mapping; full-range stereo sums fit without clipping tall peaks.
- Palette selection now applies through a deferred skin reload.
- Scrolling title foreground remains readable in the dark skin.
- Drops outside a visible deck no longer load a track into an unintended deck.

### Documentation and validation

- Added a control-connection audit, feature backlog, stem feasibility notes and
  local validation records; credited Pioneered by ntamas in the README.
- Local ARM64 builds, 26 targeted native regression tests, XML checks and VNC
  checks cover the feature work. The final preview/ruler follow-up was rebuilt
  and visually checked in RGB, FILT and 3 BAND, including both palettes.
- Pi USB identity, thermal readings, sustained audio and physical touch validation
  remain deferred. Native stem mixing is not implemented by this update.

For earlier changes, see [the differences ledger](docs/DIFFS_FROM_BASE.md).
