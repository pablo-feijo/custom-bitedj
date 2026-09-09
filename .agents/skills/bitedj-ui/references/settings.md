# Settings and Browse control maps

[Codex setup](../../../../docs/CODEX.md). Read this guide when its task trigger applies.

## A. Top Tab Bar (`topbar.xml`): `y=0..40`

- `PLAY` (Overview): `x=100, y=20`
- `BROWSE` (Library): `x=300, y=20`
- `SAMPLER`: `x=500, y=20`
- `LEVELS`: `x=700, y=20`
- `SETTINGS`: `x=900..970, y=20` (recommended: `x=950, y=20`)

## B. Browse / Library Navigation

At 1024×600, compact folder rows are 28px high with 20px indentation.
With a nonempty Prepare queue, Computer and Quick Links expanded: Prepare `(150,55)`, Computer
`(150,83)`, Quick Links `(150,111)`, Music `(200,139)`, Removable Devices
`(180,167)`, History `(150,195)`. Expanded children shift later rows by 28px.
Prepare is absent when its underlying queue is empty, including at startup.
In that case all subsequent rows shift up by 28px: Computer `y=55`,
Quick Links `83`, Music `111`, Removable Devices `139`, History `167`.
A search with no results does not hide a nonempty queue.
Confirm the current fixture from a screenshot before interacting.
Tap a grouping row to expand/collapse. For folders with tracks and subfolders,
tap the indentation cell to expand; tap the label to open tracks.
Arrow-cell centers are `x=11` for roots, `31` for their children and `51`
for grandchildren. Drag vertically to scroll without selecting.

The track table's **Folders** button `(986,56)` restores navigation using
`[Sidebar],sidebar_visible`; opening a folder sets it to 0. Table headers
use 9px bold text, 8px horizontal padding and centered vertical alignment
in an 18px band at `y=73..90`. Compact track centers are `y=102,124`
(22px spacing). Column visibility, sorting and sizes retain their saved values.
The toolbar is 32px high with 11px button text. **+ Queue** `(683,56)`
uses `[Library],AutoDjAddBottom` for selected tracks; **Queue All**
`(780,56)` uses `[Library],AutoDjAddAll` for every displayed track in the current
order, retaining selection. Clear search first to include the whole playlist.
Both append without starting playback. **Auto Play OFF/ON** `(892,56)` binds
`[AutoDJ],enabled`; with an empty queue and two loaded decks it automatically
queues the loaded tracks, keeping the playing deck first. Switching off leaves the decks playing under manual control.
**Auto DJ** appears above Prepare only while its queue is nonempty or playback
is automated, shifting later roots by 28px. Tap its label to open the queue;
its arrow expands Crates. Queue playback uses the existing Auto DJ transition
settings. Its extra Fade/Skip/transition/Shuffle/Random/Repeat row is hidden
when Auto Play is off and shown when on. This adds a 22px row above the queue
header while active; the main Auto Play toggle remains visible. Prepare remains
the separate saved manual queue.
See [GUI testing](../../../../docs/GUI_TESTING.md#browse-touch-navigation).

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
Off column. See [the current Library screenshot](../../../../docs/UI_SCREENSHOTS.md#settings-library).

## F. Levels Page (`x=700, y=20`)

- Master EQ buttons: `FLAT (x=845, y=240)`, `MODE (x=940, y=240)`.

## Service Deck Jog Smoothing

Service Preferences → Decks → Deck options adds Jog-wheel smoothing after
Clone deck (eighth row), stored as `[Controls] JogWheelFilterLength` (6, 1–64).
At 1024×600 with the native service window maximized, the spin box is `(600,302)`;
Apply is `(974,577)`. Full option order, verified coordinates and persistence
checks are in [GUI testing](../../../../docs/GUI_TESTING.md#service-deck-preferences-jog-smoothing).

## Settings deck-preview metadata

General, Library, Device, Audio, System and Info retain the deck footer.

Queue feedback: no selection and unsupported views show an in-skin explanation;
success shows the added-track and pending-queue counts. **View Queue** `(598,56)`
uses `[AutoDJ],show_queue` to open Auto DJ directly, including an empty queue.
Saved playlists appear under **Folders → Playlists** when any exist; Rekordbox
playlists remain under their source. Open the playlist, then tap **Queue All**
to append its displayed tracks. Clear search to include the full playlist.
The desktop regression also seeds a saved playlist and queues both its entries.

Key and BPM badges both have a 20px height and centered 11px text, with
38px and 54px widget widths. Traditional and Camelot keys fit the same badge;
shifted key and altered BPM retain their existing highlight controls.
The shared deck template accepts `key_badge_size` and `bpm_badge_size`;
other pages pass their original dimensions. Scope footer QSS through
`#Settings_Singleton`: the singleton parser replaces the root `Settings` ID.
PAD FX still hides this footer.

## G. System and Info dashboard

System retains its saved stack index 3 and Info retains index 5. Screen rotation,
Night/Day, network controls and Advanced preferences keep their existing keys.
The last System row is ordered **Overclock**, **Advanced**, **Power**:
`[System],overclock`, `[Master],show_preferences`, `[System],power_menu` are
momentary actions. The footer shows only the full Custom Bite DJ version, with
BiteDJ by Team Deckshark and Mixxx attribution as plain text below it. The old `[System],shutdown` and `shutdown_arm` controls remain
available for compatibility; the new menu does not repurpose their values.

Power opens a full-screen menu: Restart BiteDJ, Restart system, Power
off, Back. Each power action opens a separate confirmation with Cancel. Restart
BiteDJ flushes application state before relaunching; reboot and power-off are OS
requests with visible errors. Overclock uses Save for next restart and a separate
Restart system confirmation. Firmware defaults clears the three editable boot
overrides. Unsupported configurations disable saving.

Tap the Local Time card on Info to open Local date & time. Field order: live
preview, Region, City / timezone, automatic sync, then manual date and hour/minute
controls when sync is off. Timezones come from the installed IANA database;
changing selection previews the local day/hour before Apply. The date button
opens a fullscreen calendar with Cancel, Today and Use date. Manual changes are
sent only when a date/time control was edited. Selecting only a timezone never
sends a manual date/time. Apply sets the timezone before sync and manual time;
a failed timezone request stops the sequence. These native dialogs do not change
persisted skin enum values. See [service behavior](../../../../docs/INFRASTRUCTURE.md#touch-datetime-boot-clocks-and-restart).

Verified 1024×600 targets: System tab `(805,60)`; Overclock `(634,448)`,
Advanced `(783,448)`, Power `(933,448)`. Info tab `(951,60)`;
Local Time card `(760,180)`. Power menu buttons are centered at `x=512`,
`y=376/436/496/556` in the order above; confirmation Cancel `(267,556)`
and action `(760,556)`. Overclock Back/Defaults/Save are at `y=496`,
`x=186/512/839`; Restart system `(512,556)`. Status wrapping can move field
rows, so locate +/- from the current dialog rather than reusing row heights.

The Power label uses 12px type within the unchanged 150×44 touch target.

Timezone editor (1024×600): Region `(592,160)`, City `(592,222)`, automatic sync
`(35,282)`. In manual mode, Change date `(512,340)`; Hour minus/plus
`(222,394)/(476,394)`, Minute minus/plus `(710,394)/(964,394)`. Bottom
Cancel/Apply remain `(267,556)/(760,556)`. Calendar actions: Cancel `(186,556)`,
Today `(512,556)`, Use date `(840,556)`; previous/next month `(62,84)/(962,84)`.

Region/city lists support native touch scrolling. Clock and overclock writes use
noninteractive sudo as needed; BiteDJ itself retains the normal user account.

Touch System dialogs use independent fullscreen native windows, matching Preferences,
so Wayland cannot stack the skin’s embedded GL surfaces over their controls.
They inherit the initiating skin’s appearance and close when its owner is destroyed.
The owner window is explicitly hidden while the dialog is open to unmap embedded
GL surfaces on the Pi, then restored with its previous window state on return.

Clock selection lists countries and a curated shortlist of main cities. Qt tzdata
supplies offsets/DST; an existing timezone outside the shortlist remains selectable.

Auto Play assigns visible BiteDJ decks 1/2 to the left/right crossfader sides
before starting. An inherited center assignment must never choose hidden decks
3/4. Hidden-deck playback prevents startup; other skins retain their routing.
The desktop fixture starts deck 1 centered to cover the physical-Pi regression.

On the Auto DJ queue itself, the compact toolbar shows **Remove** `(598,56)`,
**Move Up** `(683,56)`, and **Move Down** `(780,56)` instead of queue-add actions.
These use `[AutoDJ],remove_selected`, `move_up`, and `move_down`; `queue_view`
tracks native view visibility. Select a pending entry, then move or remove it.
Selection follows the moved entry; moves at either end do nothing. These actions
work with Auto Play on or off and use the existing playlist model, including its
next-track reload signal. Removing entries preserves the source playlist and file.
The desktop regression checks entry IDs/order, duplicates, boundaries, live
playback, and deleting the final pending entry.
