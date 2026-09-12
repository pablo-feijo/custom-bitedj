# Responsive display and four-deck plan

Status: active roadmap and consolidated next-work index. Phase 0 is implemented
on the current task branch and awaits integration; later items require separate approval,
implementation, validation and release work. Technical guides retain their
detailed contracts and evidence; planned work and status are tracked here.

## Goals

- Keep the original 1024×600 HDMI touchscreen fully usable.
- Make 1280×720 landscape on Raspberry Pi Touch Display 2 use its full canvas.
- Scale cleanly to larger landscape displays without stretching touch controls
  beyond useful sizes or leaving fixed-width content stranded in the center.
- Make decks 3/4 usable without breaking saved two-deck controls, the DDJ-400
  workflow, audio reliability or the current two-deck default.

## Current baseline and constraints

- The appliance profiles are 1024×600 HDMI and portrait-native 720×1280 DSI
  rotated to 1280×720 landscape. The denser DSI panel uses a profile-level 1.20
  Qt scale while HDMI remains at 1.00. See [build and deployment](BUILD_AND_DEPLOY.md).
- BiteDJ's skin provisions four engine decks through `[App],num_decks=4`, but
  presents only Channel 1/2 in Play, summaries, routing and settings. Engine
  availability is not four-deck UI support.
- The legacy XML skin has expanding/fixed size policies but no declarative
  screen-width breakpoint primitive. Automatic profiles therefore need a small
  native layout control or separately selected skin roots; scattered width
  guesses in XML are not acceptable.
- The current architecture deliberately restricts the shipped UI and DDJ-400
  routing to two decks. Four-deck work must update that rule as one reviewed
  feature, not arrive piecemeal through copied upstream layouts.
- Touch targets remain at least 44px. Existing `[Tab]`, `[FxPanel]`, settings
  stack indices, Pad FX mode IDs and saved control keys remain stable.
- Two extra visible waveform renderers can materially affect Raspberry Pi GPU,
  CPU and USB-load headroom. Four-deck qualification needs measured frame/audio
  results on the target Pi, not screenshots alone.

## Target layout profiles

| Profile | Reference canvas | Default deck presentation | Layout policy |
| --- | --- | --- | --- |
| Compact | 1024×600 | Decks 1/2 | Preserve current geometry and coordinate regressions. Decks 3/4, when enabled later, use an explicit deck-layer switch rather than smaller touch targets. |
| Wide | 1280×720 at 1.20 scale | Decks 1/2 | Preserve readable physical density across skin and native menus, expand content lanes, and keep bounded touch panels anchored to an edge. A future 3/4 layer uses the same geometry. |
| Large | 1600×900 and above | User-selectable 2 or 4 | Use two deck rows or a 2×2 waveform grid while capping control widths and distributing surplus space to waveforms/library content. |

Thresholds are logical pixels after compositor rotation and scale. Before Phase
1 is implemented, validate 1366×768, 1600×900 and 1920×1080 to confirm the
proposed Large threshold rather than treating this table as an implementation.

## Stable control model

- Keep two decks as the default on every existing installation.
- Reuse Mixxx's established `[Skin],show_4decks` as the saved two/four-deck
  choice if its startup behavior proves compatible. Do not create a competing
  boolean.
- Add one saved layout-profile control only if needed: Auto, Compact, Wide or
  Large. Auto derives from logical screen size; a manual override supports
  unusual panels and compositor scaling.
- On Compact/Wide, expose a runtime 1/2 versus 3/4 layer. Switching layers must
  release touch holds, close incompatible drawers and never stop or reroute audio.
- On Large, four-deck mode may show all decks simultaneously. Hiding decks must
  suspend unnecessary repaint work while preserving playback and controller state.

## Work phases

### Phase 0 — current dual-resolution repair

- Replace Play's fixed 820px waveform region with an expanding region beside
  the fixed 204px FX/Key/Jump/Grid panel.
- Prove unchanged 1024×600 geometry and full-width 1280×720 Play in Night/Day.
- Audit Browse, Sampler, Levels, all Settings pages, dialogs and the cue drawer
  at both resolutions. Update the current control maps and gallery evidence.
- Apply 1.20 UI density to the DSI profile and its manual restart paths while
  retaining 1.00 for HDMI; mirror the profile in the owned GUI launcher.

### Phase 1 — responsive layout foundation

- Expose logical screen width/height and resolved layout profile to the skin at
  startup and after display changes. Keep the decision outside paint paths.
- Add fast contracts for profile thresholds, saved override migration and XML
  root selection. Extend the owned GUI launcher and desktop harness to accept a
  geometry matrix instead of embedding 1024×600 assertions.
- Replace remaining canvas-specific widths with expanding lanes or capped
  controls. Keep profile-level physical-density scaling centralized; do not
  add scattered per-widget scale guesses.
- Qualify Compact/Wide/Large in Night and Day, including native fullscreen
  system dialogs, noVNC scaling and rotated DSI input mapping.

### Phase 2 — four-deck UI shell

- Template Play waveform/source/transport blocks for Channels 1–4 without
  changing Channel 1/2 control IDs.
- Add the Compact/Wide deck-layer control and Large 2×2 presentation. Make the
  active/focused deck unmistakable and keep all four playing states visible.
- Extend bottom summaries, source/recording badges, time modes, Grid/Key/Jump,
  Beat FX routing, cue drawers and deck-load drop targets to Channels 3/4.
- Extend Settings General orientation and per-deck options. Extend the Pad FX
  editor selector to all four decks; its native settings layer already has
  four-deck coverage, but the visible editor and touch routing still need proof.
- Define Instant Doubles pairings (1↔2 and 3↔4 initially) and prevent ambiguous
  “opposite deck” behavior when a different layer is focused.

### Phase 3 — controller and workflow semantics

- Keep the DDJ-400's physical two-deck mapping on Channels 1/2 by default.
  Design and document an explicit, hard-to-trigger layer gesture before routing
  it to Channels 3/4; include LEDs, held controls, jog/scratch, Sync and Pad FX
  ownership in the transition contract.
- Decide which pair Auto DJ owns. The initial four-deck release should retain
  Channels 1/2 unless a separate pair selector is designed; it must not silently
  pick a hidden/center-routed deck.
- Generalize library load, custom drag overlay, Clone, controller Load, source
  removal and safe-eject audits to all visible/playing decks.
- Specify Beat FX policy: one shared unit with four routing buttons, or focused
  per-deck units. Preserve existing unit/preset IDs until that decision is made.

### Phase 4 — performance and release qualification

- Measure steady and worst-case render time with 2 and 4 loaded/playing decks in
  RGB, Filtered and 3 Band modes on the target Pi. Include phrase/cue overlays.
- Repeat USB load-while-recording, DDJ PCM/MIDI recovery and audio-gap analysis
  with four active decks. No logged underrun is not sufficient evidence.
- Run fast, focused native, removable and desktop suites at every supported
  geometry. Add native touch sequences through visible child widgets for layer
  switches, drawers, loads and cancellation/drag-out paths.
- Refresh the active release gallery for each supported profile, record exact
  binary/image provenance and perform physical checks on HDMI and Touch Display 2.

## Consolidated follow-up backlog

This table merges actionable follow-ups previously scattered across technical
documents. A source document remains authoritative for behavior and test detail;
this table owns priority and planned-work status.

| Order | Work item | Status and exit gate | Technical source |
| --- | --- | --- | --- |
| 1 | Finish dual-resolution Play and page audit | Software-complete on `codex/pi-ssh-default`: 1024×600 and 1280×720 Night/Day checks, gallery evidence, fast tests and desktop E2E passed. Physical-panel qualification remains item 3. | [GUI testing](GUI_TESTING.md), [UI map](../.agents/skills/bitedj-ui/references/play.md) |
| 2 | Add reusable geometry-matrix testing | Planned in Phase 1. Replace embedded 1024×600 assumptions where a test is meant to be resolution-independent; retain a canonical compact coordinate suite. | [Test strategy](TESTING.md), `tests/e2e/`, `scripts/test/` |
| 3 | Qualify the current appliance on physical hardware | Pending. Check DSI/HDMI touch mapping and spacing, thermal identity, USB topology, controller/audio rescans, Wi-Fi/Bluetooth launch, eject/power, and sustained audio under load. | [Control audit](CONTROL_AUDIT.md), [build/deploy](BUILD_AND_DEPLOY.md) |
| 4 | Complete DDJ-400/Pad FX hardware verification | Pending. Verify relative Beat FX selection, both Shift sides, Pad FX timing/audio/LEDs, disconnect cleanup and the second Pad FX bank on the physical controller. | [DDJ-400 mapping](DDJ400_MAPPING.md), [Pad FX testing](PAD_FX_TESTING.md) |
| 5 | Implement responsive profiles and visible decks 3/4 | Planned in Phases 1–4. Exit only through the layout, controller, storage/audio and release gates in this document. | This roadmap; [architecture](../.agents/skills/bitedj-workflow/references/architecture.md) |
| 6 | Remove remaining cold-load synchronous paths | Backlog. Move metadata/ANLZ parsing, rating writes and sampler-bank persistence only with DAO/thread-affinity, cancellation and safe-eject coverage. | [Recording/storage behavior](RECORDING.md), [architecture](../.agents/skills/bitedj-workflow/references/architecture.md) |
| 7 | Close semantic desktop coverage gaps | Backlog. Add result assertions for preview analysis and custom-overlay drag/drop journeys; keep native tests as the primary behavior layer. | [Review findings](TESTING.md#review-findings-and-remaining-coverage) |
| 8 | Resolve ARM64 libmad reference mismatch | Investigation backlog. Determine whether the sample difference is decoder/toolchain behavior or a reference problem; do not weaken or exclude the test to obtain green results. | [Test validation record](TESTING.md#review-findings-and-remaining-coverage) |
| 9 | Scope stems as a separate engine integration | Deferred research. Pin an upstream base, reconcile caching/engine/metadata/time-stretch changes, review GPL attribution, then pass stereo and Pi performance gates before exposing controls. Live separation stays out of scope. | [Stem feasibility](STEM_FEASIBILITY.md) |
| 10 | Review optional upstream modernization | Unscheduled. Scope library/database, BPM/key analysis, codecs and non-DDJ-400 controller updates as separate features with their own compatibility and hardware requirements. | [Differences from base](DIFFS_FROM_BASE.md) |

Historical release validation, completed features and routine post-merge CI are
not roadmap items. Their records stay in release/test documents. When a backlog
item starts, create its isolated task/worktree checklist, link it here and mark
the row in progress; mark it complete only after its stated exit gate passes.

## Change inventory

| Area | Likely owners | Required outcome |
| --- | --- | --- |
| Skin layout | `res/skins/BiteDJ/skin.xml`, `overview.xml`, `waveforms.xml`, `waveform.xml`, `cuepanel.xml`, `deck.xml`, `effects.xml`, `settings.xml`, `style.qss` | Profile-aware expanding layout and reusable Channel 1–4 templates. |
| Native UI/control plumbing | `src/widget/`, `src/preferences/`, `src/mixer/` | Screen profile, focus/layer state, four-deck source/status/settings behavior without GUI-thread I/O. |
| Library and Auto DJ | `src/library/`, `src/widget/wtracktableview.cpp` | Visible-deck-aware loading and safe two-deck Auto DJ ownership. |
| DDJ-400 and Pad FX | `res/controllers/Pioneer-DDJ-400-script.js`, its MIDI XML, Pad FX runtime/settings | Explicit deck layering, complete release-on-switch semantics and unchanged default routing. |
| Test tools | `scripts/test/`, `tests/e2e/`, `tests/padfx/`, `tests/controllers/`, `src/test/` | Geometry matrix, real touch routing and four-deck behavior/performance evidence. |
| Appliance display | `mixxx-pi-gen`, `scripts/test/run-gui-test.sh` | Logical landscape geometry and input mapping remain aligned across HDMI/DSI and scaling. |

## Validation matrix

Every implemented phase records exact results in its task checklist.

| Canvas | Deck mode | Required visual modes | Minimum interaction coverage |
| --- | --- | --- | --- |
| 1024×600 | 2; later layered 4 | Night and Day | All tabs/settings, panel controls, drawer, load/drop and layer switch. |
| 1280×720 | 2; later layered 4 | Night and Day | Same coverage plus right-edge anchoring and full-width waveforms. |
| 1366×768 | 2 and candidate 4 | Night and Day | Breakpoint boundary and no clipped/oversized controls. |
| 1600×900 | 2 and 4 | Night and Day | Simultaneous four-deck focus/routing and touch targets. |
| 1920×1080 | 2 and 4 | Night and Day | Width caps, balanced spacing and waveform/frame-time bounds. |

For every canvas: verify visible bounds, minimum 44px touch targets, labels,
footer, picker/drawer overlays, notifications, fullscreen dialogs and no dead
gutters. Four-deck runs also verify independent load/play/cue/sync/key/grid/time,
source removal, recording indicators, Pad FX holds and layer cancellation.

## Documentation changes when phases land

- Keep exact current coordinates in [GUI testing](GUI_TESTING.md) and the
  focused maps under `.agents/skills/bitedj-ui/references/`; do not duplicate
  the roadmap there.
- Update [architecture](../.agents/skills/bitedj-workflow/references/architecture.md)
  only when four-deck implementation is approved and complete enough to replace
  the two-deck rule.
- Update [DDJ-400 mapping](DDJ400_MAPPING.md), [display/load checks](DISPLAY_LOAD_TESTING.md),
  [Rekordbox display integration](REKORDBOX_DISPLAY_INTEGRATION.md),
  [test strategy](TESTING.md), [build/deploy](BUILD_AND_DEPLOY.md), the active
  UI gallery and release notes in the same feature that changes their behavior.
- Update the consolidated backlog above whenever a source guide adds, removes or
  closes planned work. Agents must not create a second competing roadmap.

## Completion gates

Phase 0 software work is complete when both virtual display geometries pass and
1024×600 has no layout regression; physical qualification is tracked separately
above. Four-deck support is complete only when Channels 3/4 are
loadable, controllable, visible and safely releasable across touch/controller
paths; all four can play without audio/render regressions; settings migrate; the
full geometry matrix passes; and the architecture rule, maps, gallery and release
documentation describe the shipped behavior rather than this plan.
