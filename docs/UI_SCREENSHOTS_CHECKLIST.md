# UI documentation task

- Branch: `codex/ui-screenshots-docs`
- Worktree: `bitedj-ui-docs`
- Merge target: `codex/v0.0.7` (merge only when requested)
- Source UI: `8f87aa338f`, application version 0.0.7

- [x] Create an isolated feature worktree from the integration branch.
- [x] Build ARM64 and launch an owned 1024×600 GUI instance.
- [x] Capture and visually inspect Play, Browse with previews, and all seven Settings tabs.
- [x] Add curated images and captions to the gallery and README.
- [x] Link previews from the changelog and require future agents to refresh them.
- [x] Verify image dimensions, Markdown links and task commit messages.

Generated fixtures, raw captures, build logs and capture provenance remain in
ignored `test-results/`. Selected publication images belong in `docs/images/ui/0.0.7/`.

Validation: ARM64 build passed; all nine images visually inspected and verified
as 1024×600 PNGs (618,404 bytes total). Documentation file links/anchors,
`bash -n` on the capture script, and `git diff --check` passed. Corrected the
stale Library settings coordinate/visibility map from the live screenshot.

Owned instance: `bitedj-gui-718691038`, left on Play. Discover its current
endpoints with the settings helper and `docker port`; launch ports can change.
No UI source or persisted control values changed. No merge or release requested.
