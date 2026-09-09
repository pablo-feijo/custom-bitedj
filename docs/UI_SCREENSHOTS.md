# BiteDJ 0.0.7 UI gallery

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This guide describes [Custom Bite DJ](../README.md), an independent fork of
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), based on Mixxx.

Synthetic **1024×600** captures in Night and Day mode. The main preview is
`0.0.7-codex-system-dashboard.10` for System and the clock dialogs; waveform/picker examples also use
`0.0.7-codex-system-dashboard.3` with the same skin. Unchanged drawer examples use
`0.0.7-codex-bottom-pad-touch.2`.
[Capture provenance](images/ui/0.0.7/semver-capture-provenance.json) records
binary and image hashes.

See the [0.0.7 changelog](../CHANGELOG.md#007--unreleased) for the changes behind
these screens, or [return to the README](../README.md).

[Grid](#grid) | [Drawer](#controller-pad-drawer) | [Play](#play) | [Beat FX picker](#beat-fx-picker) | [Browse with previews](#browse-preview) | [General](#settings-general) | [Library](#settings-library) | [Pad FX](#settings-pad-fx) | [Device](#settings-device) | [Audio](#settings-audio) | [System](#settings-system) | [Info](#settings-info)

<a id="play"></a>

## Play

Two decks with scrolling waveforms, a shorter overview ruler with smaller labels,
and the FX, Key, Jump and Grid panel.

![Play](images/ui/0.0.7/play.png)

## FX controls

Deck routing, activation, supported beat periods and effect parameters.

![All Echo parameters and native state buttons fit without scrolling](images/ui/0.0.7/fx-parameters.png)

<a id="grid"></a>
<a id="grid-key-controls"></a>

## Grid and Key

Per-deck beat-grid editing and key adjustment. Key uses two compact deck
sections, a centered current-key badge, semitone step labels and secondary
Match/Reset actions. Key and the Settings previews were refreshed with
`0.0.7-codex-browse-header-navigation.5`
([capture provenance](images/ui/0.0.7/key-settings-capture-provenance.json)).
[Control mappings](GUI_TESTING.md).

![Per-deck Grid controls](images/ui/0.0.7/grid.png)
![Per-deck Key controls](images/ui/0.0.7/key.png)

<details><summary>Day mode</summary>

![Grid in Day mode](images/ui/0.0.7/grid-day.png)
![Key in Day mode](images/ui/0.0.7/key-day.png)
![Echo in Day mode](images/ui/0.0.7/fx-echo-day.png)

</details>

<a id="beat-fx-picker"></a>

## Beat FX picker

Choose from 25 Standard effects or Saved presets. [Native approximations](BEAT_FX.md)
explain differences from Rekordbox. Navigation and Close preserve selection;
Erase clears the active effect without deleting its preset.

![Standard Beat FX, Night mode, page 1 with ECHO selected](images/ui/0.0.7/beat-fx-page-1.png)

![Standard Beat FX, Night mode, page 2](images/ui/0.0.7/beat-fx-page-2.png)

<details>
<summary>Day mode and preserved Saved presets</summary>

![Standard Beat FX in Day mode](images/ui/0.0.7/beat-fx-day.png)

Saved keeps existing preset files and identities. The former duplicate FILTER
labels now display as COLOR FILTER and RHYTHMIC FILTER.

![Saved presets, page 1, including COLOR FILTER](images/ui/0.0.7/beat-fx-saved-1.png)

![Saved presets, page 2, including RHYTHMIC FILTER](images/ui/0.0.7/beat-fx-saved-2.png)

![Saved presets in Day mode with small text actions and no empty entry](images/ui/0.0.7/beat-fx-saved-day.png)

</details>

<a id="browse-preview"></a>

## Browse with previews

Synthetic tracks in the compact library table, with a 18px header, vertically
centered 9px labels, waveform previews and both deck overviews. The compact 32px toolbar provides
**+ Queue**, **Queue Playlist**, **Auto Play OFF/ON** and **Folders**. Open a
playlist and tap Queue Playlist to append its displayed tracks in order; clear
search first to include the whole playlist. Auto Play switches automatic mixing
on/off; disabling it leaves current deck playback intact and hides the extra
Auto DJ options row. Browse captures use
`0.0.7-codex-browse-header-navigation.7`
([Browse capture provenance](images/ui/0.0.7/browse-capture-provenance.json)).

![Browse with previews](images/ui/0.0.7/browse-preview.png)

Folder navigation uses 28px rows, 20px indentation, 18px orange source icons
and a blue selection highlight. Tapping a grouping label expands it; tapping
a track folder opens its table. Indentation cells expand nested folders.
Prepare appears only while its saved queue contains tracks. Auto DJ appears
while its playback queue has tracks or is running; its label opens the queue.

![Browse folder navigation with an empty Prepare queue hidden](images/ui/0.0.7/browse-folders.png)

![Prepare appears when its queue contains a track](images/ui/0.0.7/browse-prepare.png)

Auto DJ options are hidden while off and shown while running. When the queue is empty,
Auto Play assigns visible decks 1/2 to left/right and uses their loaded tracks, keeping any playing deck
first. If neither a queue nor both loaded decks are available, a tappable
notification explains how to start. The queue and Play indicator captures use
`0.0.7-codex-browse-header-navigation.15`
([Auto DJ capture provenance](images/ui/0.0.7/autodj-capture-provenance.json)).

![Auto DJ queue with extra options hidden](images/ui/0.0.7/browse-autodj.png)

![Auto Play on with Auto DJ options visible](images/ui/0.0.7/browse-autodj-on.png)

In the Auto DJ queue, select a pending track and use **Move Up**, **Move Down**,
or **Remove**. Reordering follows the selected entry, including duplicate tracks,
and works during playback. Remove preserves the source playlist and music file.
Play shows a compact green status badge while Auto Play is on.

![Play with Auto Play enabled](images/ui/0.0.7/play-autoplay-on.png)

![Play with Auto Play disabled](images/ui/0.0.7/play-autoplay-off.png)

The Browse toolbar adds **View Queue** and labels the whole-list action
**Queue All**. Open **Folders → Playlists → Demo Playlist**, then tap Queue All.
The confirmation reports added tracks and total pending tracks; View Queue
opens that list directly. Captures below use `0.0.7-codex-browse-header-navigation.12`
([queue capture provenance](images/ui/0.0.7/queue-capture-provenance.json)).

![Saved Demo Playlist in folder navigation](images/ui/0.0.7/demo-playlist-folders.png)

![Demo Playlist ready for Queue All](images/ui/0.0.7/demo-playlist.png)

![Confirmation after adding both playlist tracks](images/ui/0.0.7/queue-feedback.png)

![View Queue displays pending tracks](images/ui/0.0.7/queue-view.png)

<details><summary>Browse in Day mode</summary>

![Compact Browse header in Day mode](images/ui/0.0.7/browse-preview-day.png)

</details>

<a id="settings-general"></a>

## Settings — General

Mixer and playback options on the left; waveform, display and cleanup options on the right.
The Settings footer aligns key and BPM in matching 20px badges with 11px text.

![Settings — General](images/ui/0.0.7/settings-general.png)

<a id="settings-library"></a>

## Settings — Library

Column visibility and relative widths, including the Preview waveform column. ON/OFF toggles visibility independently of the XS, S, M and L width selection.

![Settings — Library](images/ui/0.0.7/settings-library.png)

<a id="settings-pad-fx"></a>

## Settings — Pad FX

Eight pad selectors and the assignment editor for Normal and Shift banks. This page uses the full Settings area and hides the deck footer.

![Settings — Pad FX](images/ui/0.0.7/settings-pad-fx.png)

<a id="settings-device"></a>

## Settings — Device

Controller discovery and device configuration. The capture instance has no physical controller attached.

![Settings — Device](images/ui/0.0.7/settings-device.png)

<a id="settings-audio"></a>

## Settings — Audio

Output routing, latency/quality selection and device rescanning. Devices shown belong to the virtual capture environment.

![Settings — Audio](images/ui/0.0.7/settings-audio.png)

<a id="settings-system"></a>

## Settings — System

USB drives, screen mode and rotation, network shortcuts, overclock settings,
Advanced preferences and Power. The power menu separates application
restart, system restart and power-off, each with confirmation. The footer
features the Custom Bite DJ version with plain-text attribution to BiteDJ and Mixxx.

![Settings — System](images/ui/0.0.7/settings-system.png)

![Separate restart and power actions](images/ui/0.0.7/power-menu.png)

The overclock editor exposes CPU, GPU and voltage offset, with firmware defaults,
Save for next restart and a separate restart confirmation. This Docker capture
has no Pi boot configuration, so editing is disabled. Save/reopen/defaults are
covered by native tests using a simulated Pi configuration.

![Overclock editor without supported hardware](images/ui/0.0.7/overclock-editor.png)

<details><summary>System controls in Day mode</summary>

![System in Day mode](images/ui/0.0.7/settings-system-day.png)
![Power menu in Day mode](images/ui/0.0.7/power-menu-day.png)
![Overclock in Day mode](images/ui/0.0.7/overclock-editor-day.png)

</details>

<a id="settings-info"></a>

## Settings — Info

Four dashboard cards show audio load, local date/time, CPU load and temperature.
Tap Local Time to choose a country and one of its main cities and preview the new hour/day.
Automatic sync sets the clock; manual mode provides a calendar and time controls. Readings describe the local
ARM64 Docker capture instance, not Raspberry Pi performance.

![Settings — Info](images/ui/0.0.7/settings-info.png)

![Timezone-first editor; service unavailable in this container](images/ui/0.0.7/clock-editor.png)

Turn automatic sync off to reveal the manual date and time controls. Select a
calendar day, use Today, or change the hour/minute with the touch buttons.

![Main cities for the selected country](images/ui/0.0.7/clock-city-shortlist.png)

![Manual date and time controls](images/ui/0.0.7/clock-manual.png)
![Touch calendar](images/ui/0.0.7/clock-calendar.png)

<details><summary>Dashboard and clock in Day mode</summary>

![Dashboard in Day mode](images/ui/0.0.7/settings-info-day.png)
![Clock editor in Day mode](images/ui/0.0.7/clock-editor-day.png)
![Calendar in Day mode](images/ui/0.0.7/clock-calendar-day.png)

</details>

## Controller pad drawer

Touch or controller navigation switches between Hot Cues, Memory, Beat Jump,
Pad FX 1, Pad FX 2 and Beat Loop. [Controls and mappings](../.agents/skills/bitedj-ui/references/pads.md#touch-drawer-navigation-and-padding-1024600).

![Beat Jump with separate previous and next buttons and balanced padding](images/ui/0.0.7/controller-beat-jump.png)

![Touchable Beat Loop pads in the padded drawer](images/ui/0.0.7/controller-beat-loop.png)

![Touchable Pad FX 1 pads and mode navigation](images/ui/0.0.7/controller-pad-fx.png)

![Pad FX 2 displays the second saved bank and remains selected after releasing Shift](images/ui/0.0.7/controller-pad-fx-2.png)

![Memory cue pads with the same header and spacing](images/ui/0.0.7/touch-memory.png)

![Pad FX drawer in Day mode](images/ui/0.0.7/controller-pad-fx-day.png)

Pad mode labels use 12px text. The fixed bank area prevents the drawer from
changing height during touch navigation.

## Refreshing these images

Follow the [capture and review procedure](GUI_TESTING.md#documentation-screenshots)
and [agent screenshot policy](../.agents/skills/bitedj-ui/SKILL.md#published-ui-screenshots). Refresh
screens affected by UI changes in the same commit and link their sections from
the changelog. Raw captures remain ignored; only reviewed publication images
are stored here. Preserve this gallery once 0.0.7 is released.

<a id="service-decks"></a>

## Service preferences: Decks

Advanced Settings → Decks adds **Jog-wheel smoothing** after Clone deck.
Default 6; range 1–64 audio callbacks. Lower values respond faster; larger values
smooth pitch-bend movement. Apply changes both decks live. Available in the service preferences window.
