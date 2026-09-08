# Approved first batch

Branch: `codex/xsploit-readme-feature-map`, based on local `v0.0.6`.
The user designated **`codex/v0.0.7`** as the eventual merge target.
Source: xsploit/bitedj at `4c1dfec590f98851159fe7a64e3348e8aad306a5`.
Approved by the user on 2026-09-08. No release bump, merge, deployment or push.

- [x] 1 / DOC-01: README identity, upstream credits, preserved background and NOTICE.
- [ ] 2 / FX-01: Programmatic effect enable/disable reaches the audio engine.
- [ ] 3 / LIB-01: Persist column order and text size; retain selection after sorting.
- [ ] 10 / RB-01: Page traversal safety and resilient DAT/EXT analysis import.

Preserve the 1024×600 layout, FX/KEY overview tabs, native Beats grid,
custom Wayland track dragging, DDJ-400 mapping and existing USB override stores.
Record implementation details, provenance and actual test results below.

## Documentation

Reorganized the README and added upstream links and NOTICE. Preserved its previous
contents verbatim under a provenance header. Local Markdown targets and
`git diff --check` passed. No application behavior changed in this step.
