# Selected fork integration todo

Branch: `codex/rekordbox-padfx-display` in `bitedj-next-batch`.
Initial base: completed first batch `180ab1bb60` (inherited `v0.0.6`).
Rebased onto local `codex/v0.0.7` at `b840bf1327`, including the other task's
deck presentation, JUMP, Info and compact-menu changes. The user subsequently
authorized working version `0.0.7`: app metadata and pi-gen image naming are
synchronized on feature branches. Pi-gen work lives on
`codex/v007-custom-defaults`, pinned by the parent gitlink; `codex/v0.0.7`
remains the later merge target in both repositories.
Eventual merge target: `codex/v0.0.7`.
The parent feature branch remains unmerged; no release tag or OS image was created.
Source review pinned to xsploit/bitedj `4c1dfec590f98851159fe7a64e3348e8aad306a5`.

- [x] 1. README identity, upstream credits, links and NOTICE (first batch).
- [x] 2. Effect enable/disable correctly reaches the audio engine (first batch).
- [x] 3. Persist library layout/text size and preserve sorted selection (first batch).
- [x] 10. Safer Rekordbox page traversal and resilient DAT/EXT import (first batch).
- [x] 13. PAD FX: compact Settings editor, independent native lanes, DDJ-400
  Normal/Shift banks, saved assignments, stop/reset, regression and 1024×600 checks.
- [x] 4. Safer replacement: configurable Lock / Fader / Stop / Live; native
  player-boundary checks and compact two-deck Settings controls.
- [ ] 5. Search: searchable All Tracks; Enter must not accidentally load.
- [x] 6. Saved Prepare queue with add, individual reorder, remove and load.
- [x] 7. Optional return to Play after a successful main-deck load from Browse.
- [ ] 8. Touch keyboard for search and text entry.
- [ ] 11. Rekordbox waveform/phrase integration in stages, with offset,
  variable-tempo, fallback and performance checks before enabling rendering.
  Decoder, validated import, compact rendering, persisted phrase visibility, and colored cue previews are implemented.
  Two-deck Day/Night and fallback checks are complete; sustained audio/underrun follow-up remains.
  See [the staged plan](REKORDBOX_DISPLAY_INTEGRATION.md).
- [ ] 16. Evaluate individual boot, recovery and storage changes against our
  appliance; record proposals before adopting OS changes.
- [x] 9. Scrolling titles and software daylight-readability review at 1024×600
  across Play, Browse, Sampler, Levels and Settings. PAD FX daylight is complete.
  Physical direct-sunlight testing remains outside the ARM64 VNC environment.

This build targets two decks only; do not adopt the fork's four-deck layout or controls.

Current priority: finish the live Rekordbox display checks.

Keep the semver branch's FX/KEY/JUMP overview tabs. PAD FX belongs under Settings. Eight pad selectors
and one editor preserve touch size at 1024×600. PAD FX hides the Settings deck
footer and expands the selected editor into that space, keeping 16px outer
horizontal / 12px vertical margins and 12px card padding. Existing VNC instances remain
running; this worktree owns separate build, install, settings and ports.

PAD FX validation and reproduction: [PAD_FX_TESTING.md](PAD_FX_TESTING.md).
Display/load validation: [DISPLAY_LOAD_TESTING.md](DISPLAY_LOAD_TESTING.md).
Items 5, 8 and 16 remain queued.

Completed supporting work: isolated feature worktrees/builds/VNC containers,
Conventional Commits, updated UI mappings, and reusable synthetic fixtures.
Reproduction procedures and generators are tracked; screenshots, recordings,
statistics and test reports remain local and ignored. The GUI is at
`http://localhost:6082/vnc.html`; other branches' instances are untouched.

- [ ] Merge into `codex/v0.0.7` later, only when requested.
