# Waveform preview fixes

- Base / merge target: `origin/codex/v0.0.7` / `codex/v0.0.7`
- Resolved base: `d7e39944d0` (freshly fetched 2026-09-09; local integration matches)
- Feature: `codex/waveform-preview-fixes`
- Worktree: `/Users/pablofeijo/Documents/bitedj-waveform-fixes`
- Binary: `0.0.7-codex-waveform-preview-fixes.2`

## Checklist

- [x] Fetch current SemVer base and isolate worktree.
- [x] Review preview loading, painting, cache and settings strategy.
- [x] Move summary reads off GUI painting, avoid full waveform decompression.
- [x] Bound cache and handle analysis progress, replacement and invalidation.
- [x] Synchronize type/palette across Browse, Play and deck overviews.
- [x] Fix overview resets and loading geometry.
- [x] Add preview and waveform rendering regression tests.
- [x] Build branch binary, run tests and verify version.
- [x] Verify owned GUI and refresh affected gallery captures.

## Review findings

Browse performs synchronous filesystem/SQL reads and decompression in paint;
it refuses previews for tracks absent from the global object cache; its pixmap
cache is unbounded and keyed without waveform identity/completion; completion
zero incorrectly exposes uninitialized samples and partial analysis is stretched
to the entire track. An unmatched painter restore corrupts caller state on misses.
The 3 Band thumbnail uses a different geometry from the deck overview, and RGB
ignores palette colors. Palette changes rebuild the entire skin. Overview resets
leave completion/scaled state behind, and image width is computed from deck
controls that still describe the previous track during loading.

## Strategy

Read summaries only, asynchronously, with a bounded cache and one pending request
per view. Results are keyed by file location, never by a mutable row number, and
never installed into the deck Track object. Paint only published summary samples;
use full-track coordinates while analysis progresses. Repaint changed visible
previews, not the entire table on every tick. Palette updates refresh waveform
widgets only. Keep EQ application deck-specific: Browse represents source audio
and cannot inherit arbitrary mixer EQ from one of two decks.

## Drawer follow-up

The screenshot is from `codex/controller-pad-drawer`, commit `5a84488ae3`,
which is not integrated into the selected SemVer base. Brought only its drawer,
controller mapping and related tests into this feature worktree as a dependency;
left its infrastructure/version changes and the original worktree untouched.
Added separate Previous/Next touch controls. Mode IDs stay unchanged; forward
order is Hot Cues (0), Memory (4), Beat Jump (2), Pad FX (1), Beat Loop (3).
Previous is the inverse and wraps; release events do not change mode.

## Validation

- Fast suite: controller Pad FX and drawer mappings plus four XML/skin contracts pass.
- Focused ARM64 native suite: 17/17 pass, including four delegate regressions,
  eight waveform rendering cases, published-cache lookup and touch mode tests.
- Desktop E2E: 4/4 pass (navigation, audio start/stop, paused waveform settings
  and web/audio transport). Paused RGB/palette round trips preserve track position.
- Owned touch GUI regression: both decks, all five labels, forward wrap and full
  reverse cycle pass. Night and Day drawer captures inspected.
- Cold-cache manual check: after restarting with the two long tracks, Browse
  asynchronously displays the unloaded 16-second fixture from home cache.
  Dragging that fixture replaces the correct deck and keeps its four sections
  in full-track coordinates. Palette changes update all three surfaces.
- Binary `--version` verified: `0.0.7-codex-waveform-preview-fixes.2`.
- Instance: `bitedj-gui-3549306340`; VNC `http://localhost:53649/vnc.html`,
  audio `http://localhost:53647/stream.mp3`, raw VNC `localhost:53648`.
- No OS image or hardware deployment. Physical Raspberry Pi/USB throughput and
  physical controller testing remain separate from container verification.

Build 1 exposed a startup-order issue during visual QA: the palette control
was published after deck widgets subscribed. Build 2 creates it before parsing
the first skin; native palette/control and paused desktop tests cover the fix.
Source/build provenance is recorded alongside the worktree-local binary.

## noVNC follow-up

Removed an invalid literal-newline/duplicate declaration injected by the GUI
Docker image. The launcher repairs cached images and versions the module graph
to invalidate broken browser caches. Added parsing checks for every noVNC JavaScript
module, plus fresh/cached/idempotent repair regressions; verified the live desktop connects in the in-app browser.
This launcher-only fix retains binary build 2 and its original provenance.

## Squash integration

Squashed into `codex/v0.0.7` on top of freshly fetched `c3b314ff54`.
Preserved the already integrated controller drawer and its tests, combined both
noVNC repair paths and retained the SemVer branch version settings. No new
integration binary was produced; the existing feature build retains its version
and provenance. The feature worktree stays available for its live GUI.
