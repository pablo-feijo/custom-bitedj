# Settings and Browse control maps

[Agent tooling index](README.md). Read this guide when its task trigger applies.

## A. Top Tab Bar (`topbar.xml`): `y=0..40`

- `PLAY` (Overview): `x=100, y=20`
- `BROWSE` (Library): `x=300, y=20`
- `SAMPLER`: `x=500, y=20`
- `LEVELS`: `x=700, y=20`
- `SETTINGS`: `x=900..970, y=20` (recommended: `x=950, y=20`)

## B. Browse / Library Navigation

These are the 44px touch-navigation fixture coordinates. The compact table
variant was also documented with breadcrumb y=40..72, header y=72..94 and
track centers y=104/126. Confirm the current fixture/layout from a screenshot
before interacting; do not mix coordinates from those captures.

At 1024×600, folder rows are 44px high. With Computer and Quick Links
expanded: Prepare `(150,63)`, Computer `(150,107)`, Quick Links `(150,151)`,
Music `(200,195)`, Removable Devices `(180,239)`, History `(150,283)`.
Rows below expanded children move by 44px per child; remeasure other trees.
Tap a grouping row to expand/collapse. For folders with tracks and subfolders,
tap the 44px indentation cell to expand; tap the label to open tracks.
Arrow-cell centers are `x=22` for roots, `66` for their children and `110`
for grandchildren. Drag vertically to scroll without selecting.

The track table's **Folders** button `(980,62)` restores navigation using
`[Sidebar],sidebar_visible`; opening a folder sets it to 0. Table headers
use 11px text with 8px padding, at `y=84..117`. Compact track centers are
`y=129,151` (22px spacing). Column visibility, sort and size controls retain
their existing values. See [GUI testing](../GUI_TESTING.md#browse-touch-navigation).

## C. Settings Sub-Tab Bar: `y=40..80`

Visible order and verified button centers at 1024×600:
- `GENERAL`: `x=73, y=60`
- `LIBRARY`: `x=219, y=60`
- `PAD FX`: `x=366, y=60` (third option)
- `DEVICE`: `x=512, y=60`
- `AUDIO`: `x=658, y=60`
- `SYSTEM`: `x=805, y=60`
- `INFO`: `x=951, y=60`

Button order is independent of the persisted WidgetStack page indices. Move
buttons by their named triggers; keep stack order stable to preserve saved tabs.
PAD FX fills the remaining screen and hides the deck footer; other tabs retain it.

## D. Settings -> General Options (`x=73, y=60`)

Verified at 1024×600. Left: mixer and playback. Right: waveform/display and
cleanup. Standard row centers are `104 + 52 * row`; Track Load uses a 58px
row with center `y=470`. All rows fit above the deck footer at `y=520`.

| Column | Option | y | Button centers x (left to right) | Control mapping |
| --- | --- | --- | --- | --- |
| Left | Crossfader | 104 | Off 374, On 458 | `[BiteDJ],crossfader_enabled`: Off=0, On=1 |
| Left | Deck 1 | 156 | A 360, None 416, B 472 | `[Channel1],orientation`: A=0, None=1, B=2 |
| Left | Deck 2 | 208 | A 360, None 416, B 472 | `[Channel2],orientation`: A=0, None=1, B=2 |
| Left | EQ Mode | 260 | EQ 374, ISO 458 | `[BiteDJ],eq_mode`: EQ=0, ISO=1 |
| Left | Jog | 312 | Vinyl 374, CDJ 458 | `[BiteDJ],vinyl_mode`: Vinyl=1, CDJ=0 |
| Left | Vinyl Brake | 364 | Off 360, Short 416, Long 472 | `[BiteDJ],vinyl_brake`: Off=0, Short=1.8, Long=3.6 |
| Left | Hot Cue | 416 | Ungated 374, Gated 458 | `[Controls],HotcueActivatePlays`: Ungated=1, Gated=0 |
| Left | Track Load | 470 | Lock 290, Fader 350, Stop 410, Live 470 | `[BiteDJ],track_load_policy`: Lock=0, Fader=3, Stop=2, Live=1 |
| Right | Phrases | 104 | Toggle 788 | `[BiteDJ],show_phrases`: Off=0, On=1 (default); native two-state toggle |
| Right | Wave | 104 | RGB 872, Filt 928, 3 Band 984 | `[Waveform],waveform_type`: RGB=17, Filt=19, 3 Band=25 |
| Right | Apply Waveform EQ | 156 | On 886, Off 970 | `[Waveform],apply_eq_to_waveform`: On=1, Off=0 |
| Right | Palette | 208 | BiteDJ 886, Amber 970 | `[BiteDJ],waveform_palette`: BiteDJ=0, Amber=1 |
| Right | Key | 260 | Camelot 886, Trad 970 | `[Library],key_notation`: Camelot=3, Trad=4 |
| Right | Grid | 312 | Compact 886, Detail 970 | `[Library],grid_layout`: Compact=0, Detail=1 |
| Right | Clear | 364 | Cache 872, Cues 928, Meta 984 | Cache: `[Library],clear_cached_waveforms`; Cues: `[Library],clear_cue_overrides`; Meta: `[Library],clear_meta_overrides` |
| Right | Played | 416 | Reset 932 | Reset: `[Library],reset_played_tracks` |
| Right | Return to Play | 470 | Off 886, On 970 | `[BiteDJ],return_to_play`: Off=0 (default), On=1; successful main-deck Browse loads only |

General Settings typography: labels 12px, segment/action text 11px; button
geometry and the coordinate mappings above are unchanged. This leaves clearance
for “3 Band” on the 1024×600 display.

## E. Settings -> Library Options (`x=219, y=60`)

Configures column visibility (independent ON/OFF toggle) and widths (`XS | S | M | L`).
Verified against the documentation capture at 1024×600, UI source `8f87aa338f`:
- **Left Column**: ON/OFF `x=304`; XS `356`, S `396`, M `436`, L `476`.
- **Right Column**: ON/OFF `x=820`; XS `868`, S `908`, M `948`, L `988`.
- Row centers are `y=102 + 48 * row`:

| Row | y | Left | Right |
| --- | --- | --- | --- |
| 0 | 102 | # | Genre |
| 1 | 150 | Title | Year |
| 2 | 198 | Artist | Color |
| 3 | 246 | Album | Rating |
| 4 | 294 | BPM | Played |
| 5 | 342 | Key | Comment |
| 6 | 390 | Time | Preview |

Visibility uses `[Library],column_visible_<column>` (Off=0, On=1); width
uses `[Library],column_weight_<column>` (XS=1, S=2, M=3, L=4). Selecting a width does not enable an
Off column. See [the current Library screenshot](../UI_SCREENSHOTS.md#settings-library).

## F. Levels Page (`x=700, y=20`)

- Master EQ buttons: `FLAT (x=845, y=240)`, `MODE (x=940, y=240)`.

## Service Deck Jog Smoothing

Service Preferences → Decks → Deck options adds Jog-wheel smoothing after
Clone deck (eighth row), stored as `[Controls] JogWheelFilterLength` (6, 1–64).
At 1024×600 with the native service window maximized, the spin box is `(600,302)`;
Apply is `(974,577)`. Full option order, verified coordinates and persistence
checks are in [GUI testing](../GUI_TESTING.md#service-deck-preferences-jog-smoothing).
