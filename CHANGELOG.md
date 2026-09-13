# Changelog

Notable changes to Custom Bite DJ, an independent fork of Team Deckshark's
BiteDJ based on Mixxx. Versions use Semantic Versioning.

## [0.0.8] — Unreleased

### Added

- Add a two-deck Training mode with a single Off/Line/Boxes selector. Line and
  Boxes replace the stacked scrolling waveforms, retain the compact bottom
  overview waves plus time/pitch/bar position,
  and mask every deck/Grid BPM as `?.?` with temporary press-and-hold reveal.
- Move Grid layout, Played Reset and library-data clearing into a balanced
  Library settings footer; align the General settings columns and simplify the
  Phrases state label.

### Fixed

- Smooth the Training phase line with the shared waveform frame clock and a
  single interpolated audio position, avoiding independently sampled beat timing.
  Paint the bounded phase area within that tick to avoid queued repaint delays.
  See the [Training gallery](docs/UI_SCREENSHOTS_0.0.8.md#training-mode).
- Make overflowing deck titles wrap continuously after their initial pause
  instead of snapping back to the start.
- Add a deliberate safety gap between Quantize/Lock and Play/Cue while
  preserving 44px touch targets and saved controls.
- Mirror each deck's elapsed/remaining selection in the compact overview time
  watermark and progress shading direction.
- Keep three-digit Training bar numbers visible instead of clipping their
  leading digit after bar 99.

- Boot freshly flashed images with SSH enabled and support both the original
  1024×600 HDMI touchscreen and the portrait-native Raspberry Pi Touch Display
  2 in landscape. Keep the Normal/180° rotation control correct on both panels.
- Expand the Play waveforms across the extra Touch Display 2 width while
  retaining the fixed touch-control panel and the original 1024×600 layout.
- Scale the complete 1280x720 Touch Display 2 interface to 1.20 so fonts,
  touch targets, effect pickers and native system menus retain a readable
  physical size; keep the original 1024x600 HDMI profile at 1.00.
- Guarantee key-only remote recovery by generating unique SSH host keys before
  the SSH daemon starts on every freshly flashed image.
- Add an Info-tab SSH control that reports service availability and enables or
  disables key-only remote access immediately. Route SSH, reboot and power-off
  requests through noninteractive sudo so the touch appliance does not depend
  on a missing Polkit agent.
- Replace the left D1/D2 source labels with small USB icons and wider source names.
- Show recording as a red dot beside each Play source name, only on the USB receiving the
  recording, with a top-right fallback when neither deck uses it and fixed geometry; remove the ambiguous red playback arrows beside
  track titles. See the [recording indicators](docs/UI_SCREENSHOTS_0.0.8.md#recording).
- Extend deck read-ahead within the existing cache to tolerate multi-second USB
  stalls; retry a full reader request queue without repeated allocation/logging.
- Preserve recording takes on filename collisions and report write, flush, close,
  low-space and queue-overflow failures; retain partial takes with an error.

- Route touchscreen taps on the [Info clock card](docs/UI_SCREENSHOTS_0.0.8.md#settings-info)
  to its date/time editor, including taps over the displayed time label.

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
