# Selected fork integration todo

Branch: `codex/rekordbox-padfx-display` in `bitedj-next-batch`.
Base: completed first batch `180ab1bb60` (inherits `v0.0.6`).
Eventual merge target: `codex/v0.0.7`, absent when this worktree was created.
No release version bump or merge in this task.
Source review pinned to xsploit/bitedj `4c1dfec590f98851159fe7a64e3348e8aad306a5`.

- [x] 1. README identity, upstream credits, links and NOTICE (first batch).
- [x] 2. Effect enable/disable correctly reaches the audio engine (first batch).
- [x] 3. Persist library layout/text size and preserve sorted selection (first batch).
- [x] 10. Safer Rekordbox page traversal and resilient DAT/EXT import (first batch).
- [x] 13. PAD FX: compact Settings editor, independent native lanes, DDJ-400
  Normal/Shift banks, saved assignments, stop/reset, regression and 1024×600 checks.
- [ ] 4. Safer replacement: configurable Lock / Fader / Stop / Live.
- [ ] 5. Search: searchable All Tracks; Enter must not accidentally load.
- [ ] 6. Prepare list for the upcoming mix.
- [ ] 7. Optional return to Play after loading.
- [ ] 8. Touch keyboard for search and text entry.
- [ ] 11. Rekordbox waveform/phrase integration in stages, with offset,
  variable-tempo, fallback and performance checks before enabling rendering.
- [ ] 16. Evaluate individual boot, recovery and storage changes against our
  appliance; record proposals before adopting OS changes.
- [ ] 9. Scrolling titles and wider daylight-readability review. PAD FX daylight
  support is complete; scrolling titles and other pages are still pending.

Keep FX/KEY overview tabs. PAD FX belongs under Settings. Eight pad selectors
and one editor preserve touch size at 1024×600. PAD FX hides the Settings deck
footer and expands the selected editor into that space, keeping 16px outer
horizontal / 12px vertical margins and 12px card padding. Existing VNC instances remain
running; this worktree owns separate build, install, settings and ports.

PAD FX validation and reproduction: [PAD_FX_TESTING.md](PAD_FX_TESTING.md).
The remaining selected items above are queued; this commit implements PAD FX only.

Completed supporting work: isolated feature worktrees/builds/VNC containers;
Conventional Commits and updated agent guides; 40 passing native tests plus
controller and live virtual-MIDI/audio checks for PAD FX. The latest GUI is at
`http://localhost:6082/vnc.html`; other branches' instances are untouched.

- [ ] Merge into `codex/v0.0.7` later, only when requested.
