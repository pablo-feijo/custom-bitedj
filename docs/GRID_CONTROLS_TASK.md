# Grid controls task

- Base: `origin/codex/v0.0.7`
- Resolved commit: `170304b14a050758f7626f3f3dc86b87af090b72`
- Feature branch: `codex/grid-deck-controls`
- Worktree: `/Users/pablofeijo/Documents/bitedj-grid-deck-controls`
- Merge target: `codex/v0.0.7` (integration only when requested)
- Build version: `0.0.7-codex-grid-deck-controls.2`

- [x] Fetch active base and create isolated worktree; local integration matches remote.
- [x] Add five native grid actions per deck.
- [x] Verify behavior and layout at 1024×600 in Day/Night.
- [x] Update control mapping, gallery and changelog.

Placement confirmed by user: right panel alongside FX/Key/Jump; selecting Grid
changes waveform editing state. Top navigation remains unchanged.

Validation:
- Fast suite passes after updating the allowed right-panel page list.
- All five desktop E2E tests pass (bundled Node runtime on PATH).
- `python3 tests/e2e/check_grid.py` passes: both decks shift earlier/later,
  set at playhead and drag independently; returning to FX disables dragging.
- Manual BPM verification: 128.00→128.10 on Deck 1 and 124.00→123.90 on
  Deck 2, ten taps each; inverse actions restore the original values.
- Day/Night and open-drawer layouts visually inspected at 1024×600.
- Native opacity scale is 0–100; Grid uses 100 and restores configured opacity.
- [x] Final focused native drag/opacity regressions and binary verification: all four Grid/Rotary tests pass; rebuilt binary reports `0.0.7-codex-grid-deck-controls.2`.

Review used the task-owned `bitedj-gui-21471774` instance. It is retired during
local integration cleanup; its settings and verified build are preserved in the
integration worktree’s ignored `test-results/recovery/grid-deck-controls` directory.

Refreshed all published Play/FX/drawer images. Removed the unused legacy
`controller-pad-fx-shift.png` capture (no documentation references).

User authorized squash integration. Pulled SemVer with rebase; local and remote
integration initially matched `41c41ca628`. The remote advanced during preparation;
actual rebase parent is `6febdd5680215c46eb40cd5319682bddb7ec72df`, including
the new CI binary-cache rules. Rebased onto that tip and retained all new
Docker/cleanup/CI instructions, controller behavior and jog preference changes.
Documentation append conflicts retain both guides; feature build is .2.
The first native opacity test exposed a missing factory in the test fixture;
fixture setup/teardown now creates/destroys it explicitly. Hide-during-drag passed.
Combined verification is required before integration; integration source version
will be `0.0.7-codex-v0-0-7.4`. No integration binary will be relabeled.

Combined .2 fast checks and all five desktop E2E tests pass. The owned .2 GUI
passes per-deck Grid shift/set/drag checks and restores normal FX interaction.
The parallel native test build was OOM-killed while compiling an unrelated
looping-control test; retry uses one worker and the existing object/cache files.

The single-worker retry completed successfully: all four focused native tests pass.
Local squash integration is authorized; publication is not requested. The feature
branch and build-source recovery ref preserve provenance after checkout retirement.

Combined integration retains the user-validated Grid template and waveform interaction.
The FX task widens the side panel to 204px; combined geometry checks are pending.
