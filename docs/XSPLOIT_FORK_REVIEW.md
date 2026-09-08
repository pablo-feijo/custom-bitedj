# PiFlex fork review and selective adoption plan

Reviewed **2026-09-08**. This is a source/documentation review, not a runtime,
security, or hardware qualification. The user selected the first batch: DOC-01, FX-01, LIB-01 and RB-01.
Other functional candidates remain proposals. Implementation and verification
are tracked in [the first-batch checklist](XSPLOIT_FIRST_BATCH.md).

## Revisions and scope

| Item | Pinned value |
| --- | --- |
| Related fork | [xsploit/bitedj](https://github.com/xsploit/bitedj) (PiFlex edition) |
| Reviewed fork head | [`4c1dfec590f98851159fe7a64e3348e8aad306a5`](https://github.com/xsploit/bitedj/commit/4c1dfec590f98851159fe7a64e3348e8aad306a5) |
| Our base | Local `refs/heads/v0.0.6`, `3884c904b4` |
| Common ancestor | `39d39434160150890c8276c7cde03e454218cb76`, “Bite DJ 1.0-1: audio-path resilience, EQ/Isolator modes, USB-only history” |
| Working branch | `codex/xsploit-readme-feature-map` |
| Intended later merge target | `codex/v0.0.7` (user-selected); no merge or push performed by this review |

The local semver branch was used because there is no branch literally named
`semver`. At review time `origin/v0.0.6` advertised `f71bb57a09`, different from
our local base. The user subsequently selected `codex/v0.0.7` as the merge target.
Recheck its current history before eventually merging.
The existing `codex/deck-status-stems-system-info` worktree has uncommitted work;
it is excluded from this comparison.

The endpoint diff is **366 files, 28,653 insertions, 6,316 deletions**. This is
not a count of exclusively PiFlex-authored changes: both forks have diverged.
Its history also contains upstream merges and a merge revert. Use common-ancestor
history plus the current source when extracting a change; a README bullet or a
large feature commit is not a self-contained patch.

## README pattern to adopt

PiFlex’s [README](https://github.com/xsploit/bitedj/blob/4c1dfec590f98851159fe7a64e3348e8aad306a5/README.md)
opens with its own identity and links BiteDJ and Mixxx immediately. It credits
Deckshark and Alyxx near the top, preserves upstream background in a separate
file, separates inherited features from additions, links implementation notes,
and distinguishes recorded tests from outstanding hardware validation.
Its [NOTICE](https://github.com/xsploit/bitedj/blob/4c1dfec590f98851159fe7a64e3348e8aad306a5/NOTICE.md)
also gives scoped attribution for presentation ideas adapted from Pioneered.

Applied here:

1. Identify this repository as a custom BiteDJ fork and link its upstream projects.
2. Credit Team Deckshark, Alyxx, other BiteDJ contributors, and Mixxx visibly.
3. Summarize our changes using the existing local change ledger.
4. Preserve the previous README verbatim beneath a provenance header in
   [BITEDJ-UPSTREAM.md](BITEDJ-UPSTREAM.md). It already includes local build notes,
   so label it as our inherited README, not a pure upstream snapshot.
5. Separate our support link, upstream community links, and future feature proposals.
6. Add [NOTICE.md](../NOTICE.md) without replacing existing license files.

Do not adopt PiFlex branding, its hardware claims, or its test results as ours.
Credit PiFlex code individually if and when it is adopted. Credit its dependencies
and original community mapping authors too; crediting only the last fork is insufficient.

## Candidate inventory

“Investigate first” is a recommendation, not an implementation commitment.
Source paths below are relative to the pinned PiFlex tree unless described as local.
Size is a relative integration estimate, not a delivery date.

| ID | Candidate and evidence | Local overlap / adaptation | Recommendation and gate |
| --- | --- | --- | --- |
| DOC-01 | README identity, upstream links, historical description, NOTICE | Previous README mainly described the base project | **Implemented:** see first-batch validation |
| FX-01 | Programmatic effect enable publication, commit `4c1dfec590`; `src/effects/effectslot.cpp` | Our `EffectSlot::setEnabled` only sets the control; theirs explicitly publishes engine state | **Implemented:** standard-rack audio regression passes, including repeated states |
| LIB-01 | Column order, saved text size, selected-track preservation when sorting; `src/library/librarycolumncontrol.cpp`, `src/widget/wtracktableview.cpp` | We already expose column widths/visibility; touch drag is custom | **Implemented:** persisted header/text size and external selection; proxy/config regressions pass; physical touch qualification pending |
| LIB-02 | All Tracks search and Enter handling; `src/library/mixxxlibraryfeature.cpp`, `src/library/library.cpp`, `src/widget/wsearchlineedit.cpp` | Existing external library and USB cache paths must remain intact | **Candidate, medium:** duplicate identity, USB eject/reinsert, search latency, repeated Enter must not load a track |
| LIB-03 | Prepare queue; `src/library/trackset/preparefeature.cpp` | Uses a hidden local playlist (`PiFlex Prepare`), unlike portable USB metadata | **Candidate, medium:** decide local versus portable persistence; test duplicates, restart, missing USB, and custom touch drop routing |
| LOAD-01 | Lock/Fader/Stop/Live replacement; `src/mixer/deckloadpolicy.h`, `playermanager.cpp` | Adds channel-routing-aware policy to existing load protection | **Investigate first, medium:** touch/MIDI/double-click paths, legacy preference migration, missing controls, both decks; preserve current defaults until explicitly chosen |
| LOAD-02 | Return to Play after successful deck load | Changes navigation and may interrupt browsing for the second deck | **Candidate, small/medium:** configurable preference; failed loads and Preview must not change page |
| TOUCH-01 | Search keyboard via `wvkbd-mobintl` and repeatable grid controls | Requires Pi runtime integration; our overview intentionally has only FX/KEY | **Candidate, medium:** evaluate keyboard and grid actions separately; no new overview tabs; repeated/held actions stop on release or page change |
| DISPLAY-01 | Themes, Day/Night, scrolling labels, independent elapsed/remaining time | We already have waveform modes and touch time toggling; PiFlex targets 1920×1200 | **Candidate, medium:** extract labels/time preferences first; fit and measure 1024×600; avoid importing its 80/180-pixel layout budgets |
| WAVE-01 | Shared time scale for native/imported waveforms; `f2e6ecc672`, `src/waveform/` | Our `v0.0.6` has stacked-waveform renderer fixes | **Candidate, medium/high:** native display regression first; required with imported waveforms; verify beat spacing, zoom, offsets, and both renderer paths |
| RB-01 | Safer Device Library reading: full page indices, cycles/bounds, DAT-only beat/cue passes; `src/library/rekordbox/rekordboxpagechain.h`, `rekordboxfeature.cpp` | Existing reader and per-USB overrides are shared foundations | **Implemented subset:** page guards and independent DAT/EXT import; shared-path constraints and real-drive qualification remain outside this patch |
| RB-02 | `.2EX` PWV6/PWV7 waveforms and PSSI phrases; `rekordboxwaveform.h`, `rekordboxphrases.h`, `src/track/phrasealignment.h` | Adds track data, parser, analyzer policy and renderer integration | **Later, large:** RB-01 and WAVE-01 first; validate paired overview/detail, 150 Hz timing, grid edits, no USB write-back, and Pi rendering cost |
| RB-03 | Rekordbox-first versus BiteDJ-only analysis policy | Affects analyzer scheduling and protected/imported grids | **Later, medium/high:** separate policy from phrase UI; verify missing-data fallback and unload/reload semantics; never replace analysis on a currently loaded deck |
| MIDI-01 | FLX6 mapping, accelerated/focus-independent browsing, jog response/filter changes; `res/controllers/Pioneer-DDJ-FLX6*`, `src/util/rotary.cpp` | Our customized DDJ-400 mapping and engine jog behavior must be retained | **Candidate, medium/high:** hardware demand first; isolate generic browser fixes from controller-specific behavior; physical DDJ-400 regression and FLX6 testing if selected |
| PAD-01 | Private native Pad FX lanes and eight-pad assignment editor; `src/effects/chains/padeffectchain.cpp`, `src/preferences/padfxsettings.cpp` | Overlaps our existing DDJ-400 Pad FX/Release FX work; different routing/settings architecture | **Later, large:** compare effects and release semantics individually; FX-01 relevant; overlapping holds, latched cleanup, tails, routing, CPU/xruns; no PADS/CFX overview tabs |
| STREAM-01 | In-skin broadcasting and optional live metadata; `src/mixer/livemetadataserver.cpp`, `docs/live-workflow.md` | Broadcasting machinery is inherited; metadata HTTP/SSE interface is additional | **Later, medium/high:** choose broadcasting and metadata separately; local/LAN settings, bounded clients, lifecycle, CPU/audio load; routing candidates do not prove audibility |
| EDMC-01 | EDMC browser/search/download companion; `src/library/edmc/`, `edmc-companion/` | New Node/Playwright/Chromium service and persistent storage lifecycle | **Defer, large:** only after explicit product decision; authentication, cancellation, validation, full disk, duplicate downloads, service recovery and simultaneous audio load |
| USB-01 | Download destination identity, SD fallback, descriptor-pinned writes, coordinated eject | Tied substantially to EDMC; ordinary USB stores and eject already exist locally | **Defer with EDMC:** extract an independent fix only if reproduced locally; multi-drive unplug tests and no forced unmount |
| OS-01 | PiFlex image/runtime, splash, recovery, skin migration and deployment | Our appliance uses `mixxx-pi-gen`, Docker, Sway and existing deployment scripts | **Defer wholesale replacement:** evaluate specific fixes against our image generator; source-controlled runtime changes and boot/recovery checks required |
| BASE-01 | USB history worker, cue/metadata stores, EQ, sampler, existing FX/KEY, notifications | Already inherited; local `FsHistoryWorker` and its test suite are present | **Keep:** no duplicate implementation; review only demonstrable incremental fixes |

## Evidence and limits

The inventory combines PiFlex’s README and guides with endpoint file comparison
and focused inspection of `deckloadpolicy.h`, `preparefeature.cpp`, and the
`4c1dfec590` effect-state patch. Presence and documented intent are not proof of
correct runtime behavior. Other rows need detailed code review when selected.

Key source guides at the pinned revision:

- [Live workflow and metadata](https://github.com/xsploit/bitedj/blob/4c1dfec590f98851159fe7a64e3348e8aad306a5/docs/live-workflow.md)
- [Rekordbox compatibility](https://github.com/xsploit/bitedj/blob/4c1dfec590f98851159fe7a64e3348e8aad306a5/docs/rekordbox-read-compatibility.md)
- [Pad FX implementation](https://github.com/xsploit/bitedj/blob/4c1dfec590f98851159fe7a64e3348e8aad306a5/docs/pad-fx.md)
- [Storage reliability](https://github.com/xsploit/bitedj/blob/4c1dfec590f98851159fe7a64e3348e8aad306a5/docs/storage-reliability.md)
- [Presentation attribution and limits](https://github.com/xsploit/bitedj/blob/4c1dfec590f98851159fe7a64e3348e8aad306a5/docs/PIONEERED-INTEGRATION.md)

PiFlex documents outstanding physical-controller, multi-drive, and sustained
zero-xrun validation. Those results must be established on our target too.
Its Rekordbox support is for traditional Device Library exports; this is not
a promise of OneLibrary-only or desktop `master.db` support or export write-back.

## Suggested sequence

1. Review/merge DOC-01 independently after deciding the README wording.
2. Select one of FX-01, LIB-01, LOAD-01 or RB-01 and investigate a minimal change.
3. Choose workflow additions individually: Prepare, search, keyboard, display refinements.
4. If wanted, stage Rekordbox import as parser safety → time scale → imported data → policy → phrase UI.
5. Evaluate Pad FX, streaming, EDMC and runtime changes as separate projects.

For each selected item, copy this decision record into a focused document or issue:

```text
Candidate ID / desired user behavior:
Decision: candidate | selected | deferred | rejected | implemented
Source revision, original authors and exact commits/files:
Already present locally / minimal missing behavior:
Dependencies and excluded behavior:
1024x600 UI and DDJ-400 implications:
Settings migration and default behavior:
Regression checks and hardware acceptance:
Implementation branch / commit:
Target semver and rollback:
```

Use a fresh `codex/<candidate>` branch from the current semver base for each
implementation. Inspect both forks’ changes since the common ancestor; avoid
cherry-picking mixed commits that include skins, OS changes, and unrelated features.
Preserve original authorship for direct patches and document source provenance
for adaptations. Update the local change ledger and credits with completed work.
Run focused regressions plus the repository GUI/audio workflow for relevant
functional changes, then qualify on the Pi where hardware behavior matters.

This feature branch retains version **0.0.6**. Checked: `BITEDJ_VERSION`
in CMake, the pinned image-generator config (`103f95416473`, `IMG_NAME=bitedj-pi-v0.0.6`),
and flashing-script filenames agree. No release bump is needed for this review.
For a future release, synchronize all three as required by [AGENTS.md](AGENTS.md).

## Reproducing the comparison

```sh
git fetch https://github.com/xsploit/bitedj.git main:refs/remotes/xsploit/main
git merge-base refs/heads/v0.0.6 4c1dfec590f98851159fe7a64e3348e8aad306a5
git diff --stat 3884c904b4 4c1dfec590f98851159fe7a64e3348e8aad306a5
git log --oneline 39d39434160150890c8276c7cde03e454218cb76..4c1dfec590f98851159fe7a64e3348e8aad306a5 -- src/effects/effectslot.cpp
```

The fetch refreshes the named ref; pinned hashes keep this review reproducible.
No remote named `xsploit` is required by that command.
