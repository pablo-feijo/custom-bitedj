# BiteDJ 0.0.7 UI gallery

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This guide describes [Custom Bite DJ](../README.md), an independent fork of
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), based on Mixxx.

Synthetic **1024×600** captures in Night and Day mode. The main preview is
`0.0.7-codex-semver-vnc.7`; drawer examples use
`0.0.7-codex-bottom-pad-touch.2`.
[Capture provenance](images/ui/0.0.7/semver-capture-provenance.json) records
binary and image hashes.

See the [0.0.7 changelog](../CHANGELOG.md#007--unreleased) for the changes behind
these screens, or [return to the README](../README.md).

[Grid](#grid) | [Drawer](#controller-pad-drawer) | [Play](#play) | [Beat FX picker](#beat-fx-picker) | [Browse with previews](#browse-preview) | [General](#settings-general) | [Library](#settings-library) | [Pad FX](#settings-pad-fx) | [Device](#settings-device) | [Audio](#settings-audio) | [System](#settings-system) | [Info](#settings-info)

<a id="play"></a>

## Play

Two decks with scrolling waveforms and the FX, Key, Jump and Grid panel.

![Play](images/ui/0.0.7/play.png)

## FX controls

Deck routing, activation, supported beat periods and effect parameters.

![All Echo parameters and native state buttons fit without scrolling](images/ui/0.0.7/fx-parameters.png)

<a id="grid"></a>
<a id="grid-key-controls"></a>

## Grid and Key

Per-deck beat-grid editing and key adjustment. [Control mappings](GUI_TESTING.md).

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

Synthetic tracks in the compact library table, with smaller padded headers, the
waveform Preview column enabled, a 44px Folders control and both deck overviews.

![Browse with previews](images/ui/0.0.7/browse-preview.png)

Folder navigation uses 44px rows and expansion areas. Tapping a grouping label
expands it; tapping a track folder opens its table.

![Browse folder navigation](images/ui/0.0.7/browse-folders.png)

<a id="settings-general"></a>

## Settings — General

Mixer and playback options on the left; waveform, display and cleanup options on the right.

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

USB drives, screen mode and rotation, network shortcuts and appliance actions.

![Settings — System](images/ui/0.0.7/settings-system.png)

<a id="settings-info"></a>

## Settings — Info

Audio and system status. Readings describe the local ARM64 Docker capture instance, not Raspberry Pi performance.

![Settings — Info](images/ui/0.0.7/settings-info.png)

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
