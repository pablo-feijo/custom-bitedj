# Changelog

Notable changes to Custom Bite DJ, an independent fork of Team Deckshark's
BiteDJ based on Mixxx. Versions use Semantic Versioning.

## [0.0.8] — Unreleased

### Fixed

- Keep GUI/rendering workers out of real-time scheduling so controller browsing
  cannot consume the audio threads’ shared real-time CPU budget.
- DDJ-400 Shift + jog edits beat grids only in the right-panel Grid tab;
  other panels use gentler track search that accelerates with wheel speed.
- Restore native Short/Long Vinyl Brake on jog release and cancel scratches
  immediately when switching to CDJ. Add mapping and native timing regressions.
- Beat Sync now takes tempo from the pressed deck and syncs the other deck to
  it. Switching off releases both decks while retaining their adjusted tempos;
  Quantize alone does not align the decks when pressing Play.

- Start both main decks with Quantize enabled each session, while preserving
  manual Off until exit and across track loads; see the [Play capture](docs/UI_SCREENSHOTS_0.0.8.md#play).
- Prefer Rekordbox-exported browser waveform previews in the background,
  with native cached summaries as fallback; see [Browse previews](docs/UI_SCREENSHOTS_0.0.8.md#browse-preview).
- Prefer valid exported Rekordbox waveforms on deck load, with native cache
  and analysis as fallback; prevent late background analysis from replacing
  exported waveforms or flashing different browser colors.
- Preserve waveform renderer buffers when returning to an unchanged Play layout.
- Keep versioned release notes at the repository root and update guide links.

## [0.0.7] — 2026-09-09

### Added

- Per-deck Grid editing, harmonic Key Match/Reset, beat-jump controls and linked
  waveform zoom. Grid shift/BPM buttons support press-and-hold repetition.
- A touch performance drawer for Hot Cues, Memory, Pad FX, Beat Jump and Beat
  Loop, with independent deck pages and DDJ-400 mode synchronization.
- System-owned Pad FX presets, an eight-pad editor, independent effect lanes,
  and saved Normal/Shift banks.
- A paged Standard/Saved Beat FX picker with 25 named native approximations,
  focused parameter controls and controller navigation.
- Saved Prepare and Auto DJ queues; Queue All, addition feedback, Move Up,
  Move Down and Remove; visible-deck Auto Play with a Play-page status badge.
- Lock/Fader/Stop/Live track replacement policies and optional return to Play
  after loading from Browse.
- Rekordbox PWV6/PWV7 waveforms and PSSI phrase import, native analysis fallback,
  persisted phrase visibility and phrase alignment after beatgrid edits.
- USB/local/missing-media deck indicators, playback/output badges, scrolling
  titles, independent elapsed/remaining time, colored cues and waveform palettes.
- Touch clock/date editing, automatic network time, Raspberry Pi overclock
  settings with defaults/recovery, and confirmed app restart/reboot/power actions.
- An Info dashboard for audio load, CPU, temperature, clock and output status.
- Configurable jog smoothing and triple-Shift return to waveform previews.

### Changed

- Compact the 1024×600 Play, Browse, FX and Settings layouts; retain Day/Night
  modes and publish the [UI gallery](docs/UI_SCREENSHOTS.md).
- Keep high-detail waveform rendering opt-in. Use compact preview rulers,
  colored hot-cue/memory labels and a main CUE marker above overlapping markers.
- Share supported Beat periods and focused FX state across touch and DDJ-400
  controls; distinguish Color/Rhythmic Filter and document preset approximations.
- Build the pinned ARM64 Raspberry Pi OS image on Debian 13 Trixie, using the
  matching application and resources. Preserve the approved 0.0.7 boot overrides.
- Add verified binary/image versions and source provenance, incremental Docker
  builds, layered tests, semver desktop CI and isolated test instances.
- Organize build/deploy/test tools and native agent skills; clarify independent
  fork identity, upstream attribution and component licensing.

### Fixed

- Keep removable discovery and folder enumeration off the GUI thread. Batch
  large folders, bound queued rows, cancel stale scans and preserve saved BPM/key
  without importing every painted or sorted row into the internal library.
- Share Linux removable roots across Settings, Browse and Rekordbox. Read kernel
  mount metadata without waiting for a slow drive's filesystem statistics.
- Index Rekordbox track/playlist lookups and handle sparse, cyclic or deeply
  nested playlists without unbounded traversal.
- Load waveform summaries in the background, bound preview caches and preserve
  native cached colors when loading Rekordbox tracks. Refresh paused previews
  after palette, phrase and waveform changes; correct band mapping and clipping.
- Preserve touch/MIDI Pad FX ownership across overlapping holds, synchronize the
  second bank, and prevent drawer layout shifts and hidden-control interaction.
- Keep Shift+jog on beatgrid editing, resume playing decks on jog release, and
  halve/double active loops in measured steps. Preserve shifted FX selection.
- Route Auto Play through visible decks 1/2, preserve duplicate queue entries
  and ordering, and leave playback running when Auto Play is disabled.
- Preserve sorted library selection and layouts; align Wayland drag/drop targets
  and reject drops outside visible decks.
- Repair fresh/cached noVNC modules and retry desktop startup activation.
- Select the documented MP3 silence reference for the actual libmad arithmetic
  backend in native tests.
