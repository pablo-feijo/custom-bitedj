# BiteDJ 0.0.7 UI gallery

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This guide describes [Custom Bite DJ](../README.md), an independent fork of
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), based on Mixxx.

Native **1024×600** captures from the combined SemVer preview
`0.0.7-codex-semver-vnc.7`, compiled from `fd397110ab8b0e806da8e06986316a3555e00131`
on top of integration `06f3498581`. The binary reports the same product version.
Screens use synthetic Groove 128 BPM and Techno 124 BPM tracks, with no physical
controller or USB drive attached. Night and Day captures were visually reviewed.
[Capture provenance](images/ui/0.0.7/semver-capture-provenance.json) records the
binary and image hashes. Older task-only examples are omitted from this gallery.

See the [0.0.7 changelog](../CHANGELOG.md#007--unreleased) for the changes behind
these screens, or [return to the README](../README.md).

[Grid](#grid) | [Drawer](#controller-pad-drawer) | [Play](#play) | [Beat FX picker](#beat-fx-picker) | [Browse with previews](#browse-preview) | [General](#settings-general) | [Library](#settings-library) | [Pad FX](#settings-pad-fx) | [Device](#settings-device) | [Audio](#settings-audio) | [System](#settings-system) | [Info](#settings-info)

<a id="play"></a>

## Play

Two loaded decks with scrolling waveforms, deck overviews and the FX panel. KEY, JUMP and GRID share the 204px right-hand panel. Beats appear first in the scrolling parameter area; Echo shows only its supported periods, from 1/8 to 2 beats.

![Play: Two loaded decks with scrolling waveforms, deck overviews and the FX panel. KEY, JUMP and GRID share the 204px right-hand panel. Beats appear first in the scrolling parameter area; Echo shows only its supported periods, from 1/8 to 2 beats.](images/ui/0.0.7/play.png)

## FX controls

For a loaded effect, deck assignment and activation share a compact row.
Smaller controls give the parameter area enough room to show all Echo options.
Parameter lists begin directly below the assignment row; longer lists still scroll.
Mix appears once in the bottom row; native Mix/Dry-Wet rows are hidden without changing their saved values or Super links; longer parameter lists can scroll. Super appears only when a parameter is linked. Loaded continuous controls and native parameter buttons
are exposed below the Beats grid. Unavailable periods and unloaded controls are hidden.


![All Echo parameters and native state buttons fit without scrolling.](images/ui/0.0.7/fx-parameters.png)






<a id="grid"></a>
<a id="grid-key-controls"></a>

## Grid and Key

Grid retains the manually validated per-deck template controls, with single
press/release actions and waveform grid-edit interaction. Key shows each deck's
current key, ±2 semitones, native harmonic Match and Reset to the file key.

Both themes show the integrated Grid template and the current 204px side panel.

![Per-deck Grid controls.](images/ui/0.0.7/grid.png)
![Per-deck Key controls.](images/ui/0.0.7/key.png)

<details><summary>Day mode</summary>

![Grid in Day mode.](images/ui/0.0.7/grid-day.png)
![Key in Day mode.](images/ui/0.0.7/key-day.png)
![Echo in Day mode: centered Beats heading and all parameters visible.](images/ui/0.0.7/fx-echo-day.png)


</details>

<a id="beat-fx-picker"></a>

## Beat FX picker

The Standard section follows the 25-name Rekordbox 7 single-mode Beat FX list.
Every entry is a documented [native approximation](BEAT_FX.md), with differences
from Rekordbox explained in the catalogue. Selecting an effect closes the picker
and leaves FX Off until activated. Page navigation and Close preserve selection.

The picker stays inside the right FX panel. Two columns provide 88px-wide
effect buttons with 44px height, 10px labels and a 4px gutter. ECHO is selected
here. Seven rows fit per page; both pages keep the same spacing and long names
wrap inside their cells. Standard, Saved, Erase, Close, Prev and Next use
30px controls; the page counter uses 9px text. The counter and arrows sit
immediately below the options, with remaining panel space beneath them.

![Standard Beat FX, Night mode, page 1 with ECHO selected.](images/ui/0.0.7/beat-fx-page-1.png)

![Standard Beat FX, Night mode, page 2.](images/ui/0.0.7/beat-fx-page-2.png)

<details>
<summary>Day mode and preserved Saved presets</summary>

![Standard Beat FX in Day mode.](images/ui/0.0.7/beat-fx-day.png)

Saved keeps existing preset files and identities. The former duplicate FILTER
labels now display as COLOR FILTER and RHYTHMIC FILTER.

![Saved presets, page 1, including COLOR FILTER.](images/ui/0.0.7/beat-fx-saved-1.png)

![Saved presets, page 2, including RHYTHMIC FILTER.](images/ui/0.0.7/beat-fx-saved-2.png)

![Saved presets in Day mode with small text actions and no empty entry.](images/ui/0.0.7/beat-fx-saved-day.png)

</details>

<a id="browse-preview"></a>

## Browse with previews

Synthetic tracks in the compact library table, with smaller padded headers, the
waveform Preview column enabled, a 44px Folders control and both deck overviews.

![Browse with previews: Synthetic tracks in the compact library table, with smaller padded headers, the
waveform Preview column enabled, a 44px Folders control and both deck overviews.](images/ui/0.0.7/browse-preview.png)

Folder navigation uses 44px rows and expansion areas. Tapping a grouping label
expands it; tapping a track folder opens its table.

![Browse folder navigation.](images/ui/0.0.7/browse-folders.png)



<a id="settings-general"></a>

## Settings — General

Mixer and playback options on the left; waveform, display and cleanup options on the right.

![Settings — General: Mixer and playback options on the left; waveform, display and cleanup options on the right.](images/ui/0.0.7/settings-general.png)

<a id="settings-library"></a>

## Settings — Library

Column visibility and relative widths, including the Preview waveform column. ON/OFF toggles visibility independently of the XS, S, M and L width selection.

![Settings — Library: Column visibility and relative widths, including the Preview waveform column. ON/OFF toggles visibility independently of the XS, S, M and L width selection.](images/ui/0.0.7/settings-library.png)

<a id="settings-pad-fx"></a>

## Settings — Pad FX

Eight pad selectors and the assignment editor for Normal and Shift banks. This page uses the full Settings area and hides the deck footer.

![Settings — Pad FX: Eight pad selectors and the assignment editor for Normal and Shift banks. This page uses the full Settings area and hides the deck footer.](images/ui/0.0.7/settings-pad-fx.png)

<a id="settings-device"></a>

## Settings — Device

Controller discovery and device configuration. The capture instance has no physical controller attached.

![Settings — Device: Controller discovery and device configuration. The capture instance has no physical controller attached.](images/ui/0.0.7/settings-device.png)

<a id="settings-audio"></a>

## Settings — Audio

Output routing, latency/quality selection and device rescanning. Devices shown belong to the virtual capture environment.

![Settings — Audio: Output routing, latency/quality selection and device rescanning. Devices shown belong to the virtual capture environment.](images/ui/0.0.7/settings-audio.png)

<a id="settings-system"></a>

## Settings — System

USB drives, screen mode and rotation, network shortcuts and appliance actions.

![Settings — System: USB drives, screen mode and rotation, network shortcuts and appliance actions.](images/ui/0.0.7/settings-system.png)

<a id="settings-info"></a>

## Settings — Info

Audio and system status. Readings describe the local ARM64 Docker capture instance, not Raspberry Pi performance.

![Settings — Info: Audio and system status. Readings describe the local ARM64 Docker capture instance, not Raspberry Pi performance.](images/ui/0.0.7/settings-info.png)

## Controller pad drawer

The padded 150px drawer has independent Previous/Next touch buttons and a 44px
header. Drawer captures use `0.0.7-codex-bottom-pad-touch.2`. Forward order is Hot Cues → Memory → Beat Jump → Pad FX 1 → Pad FX 2 → Beat Loop;
Previous reverses and wraps. Controller mode selection and touch share the same
per-deck display state; FX 2 uses the second saved assignment bank. Pad FX, Beat Jump and Beat Loop respond to touch without
a controller. Held pads highlight and release safely when the drawer closes.
Hot Cues and Memory retain their existing cue actions. [Coordinates and mappings](../AGENTS.md#touch-drawer-navigation-and-padding-1024600).

![Beat Jump with separate previous and next buttons and balanced padding.](images/ui/0.0.7/controller-beat-jump.png)

![Touchable Beat Loop pads in the padded drawer.](images/ui/0.0.7/controller-beat-loop.png)

![Touchable Pad FX 1 pads and mode navigation.](images/ui/0.0.7/controller-pad-fx.png)

![Pad FX 2 displays the second saved bank and remains selected after releasing Shift.](images/ui/0.0.7/controller-pad-fx-2.png)



![Memory cue pads with the same header and spacing.](images/ui/0.0.7/touch-memory.png)

![Pad FX drawer in Day mode.](images/ui/0.0.7/controller-pad-fx-day.png)



Pad mode labels use 12px text. The fixed bank area prevents the drawer from
changing height during touch navigation.

## Refreshing these images

Follow the [capture and review procedure](GUI_TESTING.md#documentation-screenshots)
and [agent screenshot policy](../AGENTS.md#published-ui-screenshots). Refresh
screens affected by UI changes in the same commit and link their sections from
the changelog. Raw captures remain ignored; only reviewed publication images
are stored here. Preserve this gallery once 0.0.7 is released.

Picker actions use 10px text: Erase, Close, Prev and Next. Erase and Close
share the second header row. Erase clears the current FX without deleting its
saved preset; the reserved `---` entry stays hidden. Standard/Saved
remain labeled tabs; the 9px page counter reads `1 / 2`. Saved has 14/8 entries.

<a id="service-decks"></a>

## Service preferences: Decks

Advanced Settings → Decks adds **Jog-wheel smoothing** after Clone deck.
Default 6; range 1–64 audio callbacks. Lower values respond faster; larger values
smooth pitch-bend movement. Apply changes both decks live. Available in the service preferences window.
