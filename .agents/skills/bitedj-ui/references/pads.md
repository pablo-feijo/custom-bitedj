# Pad FX and controller drawer

[Codex setup](../../../../docs/CODEX.md). Read this guide when its task trigger applies.

## Pad FX Architecture

- System settings own Pad FX defaults, saved overrides and reset commands.
  Skins only place the optional editor; never embed presets or effect logic in XML.
- Keep the Settings editor usable at 1024×600: eight pad selectors, one visible
  assignment editor, minimum 44px touch controls, no extra overview tabs.
  Hide the Settings deck footer only on PAD FX; preserve outer and card padding.

## Controller pad drawer

The Overview cue drawer follows the DDJ-400 mode selected on either deck.
Hot Cue restores the existing Hot Cues/Memory controls; Pad FX, Beat Jump and
Beat Loop show **touchable performance pads**. Touch or use the physical
pads to perform the displayed actions; no controller is required for touch. X still closes the drawer;
selecting a controller mode opens that deck again. Mode button release does
not close it. Other modes close only their own deck's visible drawer.

Runtime controls (not saved): `[PadFX],dN_mode` = 0 Hot Cue, 1 Pad FX 1,
2 Beat Jump, 3 Beat Loop, 4 Memory, 5 Pad FX 2; `dN_shift` = 0/1; `dN_jump_bank` = 0/1/2 for
1/16, 1, 16 multipliers. The existing mapping shares jump sizes across decks.

The header has separate 48×44px Previous/Next buttons and a read-only mode
label. A bounded 92px bank area holds two 44px pad rows and a 4px gap.
Navigation coordinates and mode mappings are listed below.
See [controller drawer screenshots](../../../../docs/UI_SCREENSHOTS.md#controller-pad-drawer).

Touch pad centers retain x=`132,385,638,891`, y=`525,574` at 1024×600.
`[PadFX],dN_touch_p0..7` are momentary inputs, row-major. Pad FX reads saved
assignments on press, Beat Jump seeks once, Beat Loop holds rolls 1/4–2 and
toggles loops 4–32. Release, drawer hide, mode/Shift change and touch cancellation
release held actions. Release Echo's configured toggle remains latched until
its next press or Clear FX. `[PadFX],dN_hardware_p0..7` carries MIDI FX presses
(0 release, 1 Normal, 2 Shift) into the same system runtime, with independent
input ownership. `dN_hardware_clear` releases controller-owned FX on disconnect.
`runtime_available=1` selects this bridge; the mapping retains its old runtime
when used with older binaries. Shift Jump bank changes are shared with MIDI.

Pad FX 1 displays slots 0–7; Pad FX 2 displays slots 8–15, the existing saved
Shift assignments. FX 2 stays selected after Shift is released. Both touch
arrows and controller mode selection show these same assignments. MIDI FX 2
pads use notes `0x50..0x57` on the normal/shift pad channels, with note-on and
note-off routed to the second bank. No saved assignments or enum IDs migrate.

The legend follows saved Normal/Shift assignments, timing overrides, strength
and toggle settings. Beat Loop shows four held rolls and four toggle loops;
holding Shift shows its mapped shifted Pad FX assignments. Beat Jump's Shift
bank shows size ÷16 / ×16 on pads 7/8. Modes are independent per deck.


## Touch drawer navigation and padding (1024×600)

- Open Deck 1/2 with the existing CUE chips `(60,462)` / `(572,462)` on Play.
- Drawer header: Previous `(116,476)`, mode label `(530,476)`, Next `(944,476)`,
  Close `(994,476)`. Header targets are 44px tall, arrows 48px wide.
- Previous emits `[PadFX],dN_previous`; Next emits the existing `dN_cycle`.
  Both emit press/release, with only press advancing. The label only reflects
  `dN_mode`; it does not change mode when tapped.
- Forward order: Hot Cues=0 → Memory=4 → Beat Jump=2 → Pad FX 1=1 → Pad FX 2=5 → Beat Loop=3.
  Previous reverses and wraps. Each deck retains its independent mode.
- The 150px drawer keeps 8px horizontal and approximately 4px vertical outside
  clearance, 4px row gaps and at least 44px pads. Pad centers are approximately
  x=`132,385,638,891`, y=`525,574`; inspect current geometry before pad actions.
- Verify both directions, controller-to-touch mode handoff, Close, all six pages,
  and Day/Night. See [drawer gallery](../../../../docs/UI_SCREENSHOTS.md#controller-pad-drawer).

## Touch mode transition regression

Touch mode transition regression: banks share a stacked layout so overlapping
visibility notifications cannot add their heights or move the header. Cycle
all six modes repeatedly on both decks, including a held arrow press and
release; verify one advance per press, stable header/waveform geometry, and no
flash when entering or leaving Memory/Hot Cues. Mode IDs remain unchanged. Verify both arrows and the read-only mode label.
