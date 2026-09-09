# Changelog

Notable changes to Custom BiteDJ are recorded here. Versions use Semantic
Versioning; commit messages follow Conventional Commits.

## [0.0.7] — Unreleased

### Added

- System-owned Pad FX presets and a compact eight-pad Settings editor, with
  independent effect lanes and DDJ-400 Normal/Shift banks.
- Configurable Lock / Fader / Stop / Live track replacement behavior.
- Validated Rekordbox PWV6/PWV7 waveform and PSSI phrase import, with native
  analysis fallback and phrase alignment after beatgrid edits.
- Persisted phrase visibility, a saved Prepare queue, and optional return to
  Play after a successful main-deck load from Browse.

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
- Keep overview waveforms at 38px, with 9px rulers and discreet 10px phrase strips.
- Place PAD FX third in Settings and group General options for the 1024×600 screen.
- Use compact colored hot-cue letters and memory-cue numbers in previews;
  retain full names in Play. The orange main CUE marker takes visual priority
  over overlapping hot cues, memory cues, playheads and countdowns.
- Align application and image-generator working versions at 0.0.7 and pin the
  image generator's semver merge commit in the parent repository.
- Adopt the Custom Bite DJ identity with concise upstream credits.

### Fixed

- Phrase visibility now toggles both paused bottom previews immediately.
- Stronger colored preview cue lines remain visible above waveform shading.
- Effect enable/disable reaches the engine; library layout persistence and
  sorted selection survive updates; Rekordbox page traversal tolerates bad data.

- Clipped lower waveforms and stacked rendering outside the image boundary.
- 3 BAND preview colors, band order and baseline now match the main waveform's
  band mapping; full-range stereo sums fit without clipping tall peaks.
- Palette selection now applies through a deferred skin reload.
- Scrolling title foreground remains readable in the dark skin.
- Drops outside a visible deck no longer load a track into an unintended deck.

### Documentation and validation

- Keep UI control/position maps and agent guides synchronized. Feature branches
  have independent worktrees, builds, settings and ARM64 VNC instances.
- Retain synthetic Rekordbox and MIDI/audio fixture generators and reproduction
  procedures; exclude generated media, screenshots, logs and test reports.
- Native checks cover import, alignment, cue marks, loading and queue persistence;
  visual procedures cover two decks, overlapping cues, phrases and Day/Night.
- Sustained audio validation remains open because the virtual test device logged
  underruns. Physical Pi, touch, thermal and direct-sunlight checks remain separate.
- Integrate application and image-generator feature branches into `codex/v0.0.7`
  with explicit merge commits. No release tag, published image or flash is included.

For earlier changes, see [the differences ledger](docs/DIFFS_FROM_BASE.md).
