# Pioneered-inspired BiteDJ checklist

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

Project scope: [Custom Bite DJ](../README.md), the independent BiteDJ fork.

Reviewed 2026-09-08. This is a planning backlog, not authorization to implement
all items. Checked items exist in BiteDJ; unchecked items are proposed work.
Feature branch: `codex/deck-status-stems-system-info`.
Integrated into `codex/v0.0.7` with a merge commit. Release remains unreleased;
the application version and release tags are unchanged.

## Done on this branch

- [x] Track source: actual USB slot/volume, LOCAL and removed-media handling.
- [x] Compact ON badge driven by native playback and main-output routing.
- [x] Clear KEY/LOOP readouts and active quantize/keylock styling.
- [x] Right-side JUMP tab: per-deck size, halve/double, backward/forward.
- [x] Consistent FX/KEY/JUMP spacing; native effect beat grids preserved.
- [x] Settings → INFO: audio load, CPU, temperature, clock and output status.
- [x] Smaller deck/source and key/CUE badges; removed playback arrows.
- [x] Compact 40px top menu with smaller buttons and 10px gaps.
- [x] Local ARM64 build, 14 targeted native tests and GUI/audio checks.
- [x] Updated primary local VNC after UI changes.
- [x] Conventional Commit policy in agents.md and compliant task commits.
- [x] Stem dependency assessment documented; implementation remains open.

Evidence: [validation](DECK_STATUS_VALIDATION.md),
[original plan](DECK_STATUS_STEMS_SYSTEM_INFO_PLAN.md),
[stem assessment](STEM_FEASIBILITY.md).

## First: finish validation of the current features

Pi hardware validation is deferred at the user’s request.

- [ ] Test physical USB slot identity, unplug/replug and Pi thermal readings.
- [ ] Run sustained two-deck playback with FX on the target Pi; record callback
  load and underruns, including source removal/recovery.
- [ ] Check the smaller menu on the real touchscreen for missed/adjacent taps.
- [ ] Refresh GUI coordinate documentation and screenshot checks for the 40px bar.
- [x] Review and merge into `codex/v0.0.7` with a detailed Conventional Commit.

## Recommended next: small, useful improvements

Ordered by suggested implementation sequence. These are BiteDJ proposals based
on the reference's deck controls and presentation, not promises of direct reuse.

- [x] **Linked waveform zoom — locally verified.** Add zoom in/out/reset for both decks together.
  Put controls in a settings option or compact drawer, preserving FX/KEY/JUMP.
  Accept when both decks match after reset and controller changes remain coherent.
- [x] **Independent elapsed/remaining time — locally verified.** BiteDJ now uses persisted per-deck selection rather than the global
  `ShowDurationRemaining` in the deck strip.
  Accept when changing Deck 1 does not alter Deck 2 or move its layout.
- [x] **Long-title display.** Long titles scroll within their existing area;
  full Unicode text and stable geometry have native regression coverage.
  Keep artist and badges fixed; test long Unicode titles and empty decks.
- [x] **Overview time ruler.** Sparse minute marks beneath a full, unclipped
  38px overview, with a separate 12px ruler row.
- [x] **Waveform palette option.** Added a persisted blue/amber/cream preset. Compare
  filtered, RGB and overview rendering on identical tracks. Preserve user choice;
  do not assume a palette alone reproduces another analyser's output.

Reference: [feature overview](https://github.com/ntamas94/pioneered-by-ntamas#features)
and [skin behavior](https://github.com/ntamas94/pioneered-by-ntamas/blob/main/docs/the-skin.md).

## Improvements to investigate before scheduling

- [x] **Drop-target feedback — locally verified.** Extend BiteDJ's existing touch drag behavior with
  a clear destination highlight. Verify using its custom drag path, not only Qt drag.
- [ ] **Search keyboard behavior.** Audit the existing Wayland keyboard integration
  before adding anything; test focus, dismissal and library space on hardware.
- [ ] **Optional compact health indicator.** Reuse INFO telemetry for an unobtrusive
  warning/shortcut during playback; keep routine numbers off the main screen.
- [ ] **Output switching UX.** Review existing Audio settings first. Design clear
  pending/applied states and device-loss recovery before adding any shortcut.
- [x] **Control-binding audit — connection/handler review complete.** Inventory custom skin controls and their owners;
  flag misspelled or unwritten controls and verify visible effects actually work.

The reference's detailed skin documentation explicitly identifies disconnected
keyboard/output controls and a skin-generation script that lags the actual XML.
Treat working source and end-to-end tests as evidence, not button appearance or
README claims alone. [Reference audit](https://github.com/ntamas94/pioneered-by-ntamas/blob/main/docs/the-skin.md#pioneered-and-why-a-typo-there-is-silent).

## Larger or optional projects

- [ ] **Native stems — separate engine project.** Compare a pinned upstream upgrade
  with a backport. Prove independent mute/volume, looping, seeking and keylock on
  two decks on the Pi before designing the final controls. Pre-separated files first.
- [ ] **Four-deck layouts — only if wanted.** Prototype readability and GPU cost;
  validate repeated layout switches and preserve a strong two-deck default.
- [ ] **DDJ-1000 support — hardware-dependent.** Assess mapping, audio and jog-display
  components separately only if that controller becomes a BiteDJ target.
- [ ] **AutoDJ shortcut — optional workflow.** Review current AutoDJ behavior before
  exposing a prominent single/continuous-play switch.

These larger ideas are described in the [reference repository](https://github.com/ntamas94/pioneered-by-ntamas).
Stems must provide actual independent mixing; the reference's three render-loading
buttons are not that engine. Its separate native stem strip is version-dependent.
Do not restore PADS/CFX overview tabs. Check attribution and component licenses
before copying source; UI inspiration does not require importing their provisioning.

## Completion rule for future work

For each selected item: inspect existing BiteDJ support, define behavior, implement
on the feature branch, run relevant checks, deploy local VNC and inspect it, then
commit with Conventional Commits. Hardware-dependent items remain unchecked until
verified on the actual device. Documentation-only updates do not require a rebuild.

## Current follow-up status

- [x] Compact Browse styling: smaller headers, text and breadcrumb implemented.
- [x] Connection/handler audit recorded in [CONTROL_AUDIT.md](CONTROL_AUDIT.md).
  Hardware-dependent actions were not exercised.
- [x] 26 targeted native regression tests pass, including independent time modes,
  scrolling title rendering, palette isolation, telemetry, routing and touch scrolling.
