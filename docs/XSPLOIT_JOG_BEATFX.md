# Jog and Beat FX task checklist

- Base: `origin/codex/v0.0.7`
- Resolved base: `170304b14a050758f7626f3f3dc86b87af090b72`
- Feature: `codex/xsploit-jog-beatfx`
- Worktree: `/Users/pablofeijo/Documents/bitedj-xsploit-jog-beatfx`
- Merge target: `codex/v0.0.7` (local squash merge authorized)
- Build version: `0.0.7-codex-xsploit-jog-beatfx.1`

Requested behavior follows the xsploit/PiFlex feature review. These are local
implementations against the current Custom Bite DJ mapping, not copied fork code.
The [earlier pinned review](XSPLOIT_FORK_REVIEW.md) records source context.

- [x] Active-loop jog half/double resizing with independent per-deck thresholds.
- [x] Shift jog alignment and scratch exclusion, including dedicated MIDI notes.
- [x] Immediate jog release preserving playing/paused state.
- [x] Service preference, persistence, bounds and live filter changes.
- [x] Focused FX period/toggle and Shift all-slots off; retain selection controls.
- [x] Fast suite, including both-deck MIDI behavioral regression coverage.
- [x] Native Rotary tests: 1/6/64 impulse response, bounds and buffer reset.
- [x] ARM64 build, binary version and service preference GUI verification.

Build source: `6d37ba9504` (subsequent changes only add tests/documentation).
The binary reports `0.0.7-codex-xsploit-jog-beatfx.1`; its full source commit,
branch, platform and SHA-256 are in `dist-linux/build-provenance.json`.

Validation: fast suite passed; 21 controller behavior groups; two native
`RotaryTest` cases passed in the ARM64 builder. GUI verified default 6, both
bounds, Apply, Cancel, Restore Defaults and saved value 17 across restart.
Test setting returned to 6. [Verified service screenshot](UI_SCREENSHOTS.md#service-decks).

Owned GUI: `bitedj-gui-2468427910`; [noVNC](http://localhost:56373/vnc.html),
[audio](http://localhost:56374/stream.mp3), VNC `localhost:56375`.
No physical controller was connected; jog feel and hardware timing remain a
DDJ-400 check. Locally squash-integrated after `git pull --rebase origin codex/v0.0.7`
confirmed base `c0eb61e5116db0ec333edede9166156345b96ac7`. All newer CI and
Docker instructions were preserved; the overlapping root instructions were
combined. The integration source reserves `0.0.7-codex-v0-0-7.3` for its next
build; the verified feature binary remains at its original version and provenance.
No integration binary was rebuilt or relabeled by this merge.

Cleanup audit: original tip `4c72766f99` is retained on the feature branch.
Its 4.4 GiB worktree (3.7 GiB build cache, 576 MiB installed artifact) remains because the live review container
`bitedj-gui-2468427910` bind-mounts them. Retire that preview before removing
its checkout; preserve source/build provenance. No other task is changed.
Publication was not requested. No push or hardware deployment was performed;
post-merge CI checks apply after publication, as required by the updated agents.
The full fast suite passed again on the combined integration tree.
