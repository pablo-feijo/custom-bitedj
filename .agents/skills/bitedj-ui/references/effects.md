# Beat FX and picker control maps

[Codex setup](../../../../docs/CODEX.md). Read this guide when its task trigger applies.

## Beat FX catalogue and picker

Maintain [docs/BEAT_FX.md](../../../../docs/BEAT_FX.md) alongside factory XML and native controls.
The Standard section uses versioned `[RB7] ` IDs with documented native
approximations. Preserve legacy/custom files, Pad FX IDs and the Saved section.
Never silently claim reverse, freeze, slip or transport behavior for delay chains.
Generate factory XML with `scripts/build/generate-beatfx.py`; keep tests in
`tests/effects/` and native audio/control regressions in `src/test/`.

The picker uses two columns and seven rows per page, with fixed 44px effect
buttons, 10px labels, 4px gaps and 4px outside padding. Utility buttons are
30px high and the page counter uses 9px text;
page changes and Close must never load an effect. Match selected state to the
live chain. Refresh the [picker gallery](../../../../docs/UI_SCREENSHOTS.md#beat-fx-picker)
and Play image for visible changes, together with changelog links.

Beat grid labels are periods in beats. Bind `parameterN_beat_period`; native
code converts Tremolo's cycles/beat and mirrors clamped values. Keep existing
raw controls and persisted parameter values unchanged. Verify both Echo and
Tremolo when changing time controls, including Echo's two-beat maximum.

The `BeatFxPicker` is an explicit native exception to the kiosk's
dialog suppression. Do not remove that exception or broadly enable stock
modal dialogs. Use `HighContrast::mapStyleSheet` for its native styling and verify
both Night and Day after styling changes.

Verified picker coordinates are listed under “Panel-contained Beat FX picker” below.
Order is row-major; page 1 has entries 1–14, page 2 has 15–25. Selection uses
persisted preset IDs; page/section buttons never write `chain_selector`.

## Panel-contained Beat FX picker

The FX selector `(922,116)` opens a child picker bounded by
`x=828..1015, y=100..599`. Standard/Saved centers are `(876,119)` /
`(968,119)`, Erase/Close `(876,153)` / `(968,153)`. Effect columns are
`x=876,968`, row centers `y=194,242,290,338,386,434,482`; each target is
44px tall with 10px labels. Utility controls are 30px tall; the page counter
uses 9px text at `(922,514)`. Prev/Next: `(876,538)` / `(968,538)`. Standard pages contain
14/11 entries in row-major order. Saved identifiers and controller order
are unchanged. Page changes and cancellation preserve selection; leaving
FX dismisses the picker. See [the gallery](../../../../docs/UI_SCREENSHOTS.md#beat-fx-picker).

Picker actions use 10px text: Erase, Close, Prev and Next. Erase and Close
share the second header row. Erase clears the current FX without deleting its
saved preset; the reserved `---` entry stays hidden. Standard/Saved
remain labeled tabs; the 9px page counter reads `1 / 2`. Saved has 14/8 entries.

## Play FX parameters

Compact FX selector is (922,116); routing 1/2 and activation share y=152
at x=858/922/986. Beat buttons use x=858/922/986, y=204/236. Echo’s
Feedback/Ping Pong/Send knobs are (1001,265/295/325), Quantize/Triplets
(986,356/388), and Mix/Super knobs (902/1000,424). The parameter viewport
is y=168..404. Echo fits without scrolling; longer lists retain scrolling.
The side panel is 204px wide and the waveform area is 820px.

FX availability: hide unsupported Beat periods and unloaded controls. Super
requires a loaded, linked parameter. Check Echo, Phaser, Trans, Enigma Jet and
an unloaded chain; see [the availability regression](../../../../docs/GUI_TESTING.md#available-fx-controls).

The compact FX list omits internal `mix` / `dry_wet` parameter rows using
read-only `parameterN_is_mix` metadata. Unit Mix remains visible; saved native
values and Super linkage are preserved. Check all available Standard/Saved
presets and backend manifests, including Flanger and White Noise, after changes.

Keep `#BeatFX_ParameterList` top-aligned within its 236px viewport. At 1024×600,
Flanger's first knob center is `(1000,183)`, with subsequent rows 30px apart;
Mix stays at y=424. For scroll checks use the label area `(835,340)`, not a
knob (wheel events over knobs adjust values). Phaser's Stereo row must remain
reachable and selecting Echo must restore the top of the parameter list.
