# UI change workflow

[Agent tooling index](README.md). Read this guide when its task trigger applies.

## UI Change Documentation

- Whenever adding, removing, renaming, moving or resizing a UI option, update
  its option order, verified coordinates and control/value mappings in the relevant control map linked below
  and [docs/GUI_TESTING.md](../GUI_TESTING.md) in the same commit.
- Update affected automation coordinates, controller documentation and screenshots
  when their targets or behavior change. Prefer links to the canonical mapping
  instead of maintaining contradictory copies; never reuse stale coordinates.
- Preserve control keys, enum values and persisted WidgetStack indices for layout-only
  changes. If behavior changes, document the new mapping and any settings migration.
- Verify the changed page in the owned VNC instance at 1024×600, including labels,
  touch targets, padding and footer visibility; check Day/Night when styles change.

## Published UI Screenshots

- UI changes must refresh every affected image in `docs/images/ui/0.0.7/`
  and its caption in [the UI gallery](../UI_SCREENSHOTS.md) in the same commit.
  This includes visible labels, controls, layout, styling and waveform rendering.
- Follow [the capture procedure](../GUI_TESTING.md#documentation-screenshots).
  Use an owned 1024×600 instance and synthetic fixtures; inspect each final image.
- Link the affected gallery section from the corresponding UI changelog entry.
  Keep README previews and gallery links valid when moving or renaming images.
- Curated documentation images are committed assets, explicitly requested for
  the README/docs. Raw captures and other test-run results remain ignored under
  `test-results/`; do not commit those.
- Refresh the current unreleased version's gallery in place. When starting a new
  release, retain released images and use a new version directory and gallery so
  historical changelog links continue to show their release's UI.

## Coordinate and style verification

Never guess xdotool coordinates. Use current XML geometry or inspect a current
1024×600 screenshot; use the pixel scanning and spacing procedures in
[build and test](build-and-test.md). Wait for display-mode notifications to
clear before using the top bar. Existing maps are capture-specific evidence,
not a substitute for checking the current layout.

Read the relevant control map before editing or interacting:

- [Settings and Browse](ui-settings.md)
- [Waveforms, Grid and Key](ui-play.md)
- [Beat FX and picker](ui-effects.md)
- [Pad FX and controller drawer](ui-pads.md)

Use [GUI testing](../GUI_TESTING.md) for reproducible regression procedures and
[the gallery](../UI_SCREENSHOTS.md) for the expected published appearance.
