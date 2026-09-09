# DDJ-400 reverse effect selection

- Branch: `codex/ddj400-shift-fx-back`
- Merge target: `codex/v0.0.7` (merge only when requested)
- Worktree: `bitedj-ddj400-fx-back`

- [x] Inspect normal/shifted MIDI bindings and existing reverse handler.
- [x] Honor either deck's Shift on the normal SELECT message.
- [x] Preserve dedicated shifted SELECT, release behavior and native relative preset navigation.
- [x] Update controller documentation and changelog.
- [x] Run regression checks through the XML MIDI bindings.
- [ ] Physical controller verification (requires connected DDJ-400).

The initial controller-only fix changed no pixels. The subsequent catalogue
and two-column picker work requires refreshed Play and picker screenshots.

- [x] Generate 25 standard named native approximations with explicit limitations.
- [x] Preserve legacy/custom files and distinguish both Filter display names.
- [x] Implement two-column paged picker with 50px minimum effect targets.
- [x] Correct period/rate conversion for the on-screen beat grid and DDJ controls.
- [x] Pass native catalogue, upgrade, activation, timing and audio smoke checks.
- [x] Verify picker in the owned 1024×600 GUI, including Day/Night.
- [x] Publish refreshed Play/picker images and link docs/changelog.
- [x] Commit the completed follow-up (no integration merge).

- [x] Correct the index/direction mismatch found during the effect catalogue review.

## Squash integration

- Original feature base: `codex/v0.0.7` (integration documentation synchronized at `559edc3def`).
- Fetched integration base before squash: `2c82d82eab`.
- Completed feature tip: `b6da79d769` on `codex/ddj400-shift-fx-back`.
- Merge target: `origin/codex/v0.0.7`; user authorized squash integration and rewriting prior merge history.
- Preserve the layered E2E compatibility entry point; keep the optional FX
  recording helper separately at `scripts/test/capture-fx-smoke.sh`.
- Existing native/GUI results belong to the feature binary. This source-only
  integration does not deliver or relabel a new binary.
