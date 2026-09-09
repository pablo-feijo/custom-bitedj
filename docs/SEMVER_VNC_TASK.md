# SemVer integration and VNC testing

- Base: origin/codex/v0.0.7
- Resolved base: c3b314ff54739b01b767477bbe9f4f5f0160e248
- Staging branch: codex/semver-vnc
- Worktree: /Users/pablofeijo/Documents/bitedj-semver-vnc
- Merge/publish target: origin/codex/v0.0.7
- User authorized squash integration, monitoring other project tasks, and VNC testing.
- Preserve active feature branches and live test instances.

## Integration ledger

| Task | Source | Staged squash | Status |
| --- | --- | --- | --- |
| Fix track preview synchronization | codex/waveform-preview-fixes through b9ade4761a | 66a5e2e28b | Already merged by other task, including b9ade4761a noVNC follow-up |
| Fix browse and FX menu UI | codex/browse-fx-touch-ui | Pending | Active follow-up; wait for committed final state |

## Checklist

- [x] Fetch current base and create isolated staging worktree.
- [x] Create 15-minute thread monitor.
- [ ] Review and squash ready feature tasks, preserving existing integration changes.
- [x] Assign unique prerelease and commit build source: `0.0.7-codex-semver-vnc.2`.
- [x] Build ARM64 and verify version/provenance.
- [x] Fast suite, 16 focused native tests, and 5 desktop tests pass.
- [ ] Fast-forward publish completed squash commits to SemVer branch.
- [x] Launch/open owned VNC and record endpoints.

The waveform task predates the integrated controller drawer. Conflict resolution
keeps the current integration noVNC repair and test coverage while adopting the
new Previous/Next drawer controls and associated geometry. For repeated squashes,
apply only changes after the recorded source commit; ancestry alone cannot detect
a squash integration. Never integrate an active task's unfinished work.

External integration detected: local codex/v0.0.7 advanced to 66a5e2e28b.
Duplicate unpublished staging commits preserved at refs/backup/semver-vnc-before-external-squash;
this task now starts from the actual squash merge. Build 1 passed 16 focused native tests;
build 2 rebuilds the actual integration source with its own prerelease.
Owned instance: `bitedj-gui-3527155601`. Stable endpoints: web http://localhost:6080/vnc.html, raw VNC localhost:5900, audio http://localhost:8000/stream.mp3.
The live browser connection was verified after a fresh launcher run.
Build provenance is in ignored dist-linux/build-provenance.json.
A launcher race fix waits for Xvfb readiness before starting x11vnc.

The Browse/FX task remains active, rebasing and validating its latest changes;
wait for completion and a fresh local/remote SemVer check before integration.
On refresh, preserve stable ports with BITEDJ_TEST_WEB_PORT=6080,
BITEDJ_TEST_VNC_PORT=5900, BITEDJ_TEST_AUDIO_PORT=8000.

Validated launcher fix: clean instance recreation connected on the same stable
ports, with no manual x11vnc start. Running process hash matches the verified .2
binary. Task source and launcher changes are ready for squash publication.
