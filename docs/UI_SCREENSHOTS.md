# BiteDJ 0.0.7 UI gallery

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This guide describes [Custom Bite DJ](../README.md), an independent fork of
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), based on Mixxx.

Native **1024×600** screenshots using synthetic music in isolated ARM64 Docker
instances. Play, Browse, Settings and the drawer were refreshed on
`codex/waveform-preview-fixes`, binary `0.0.7-codex-waveform-preview-fixes.2`.
They use generated Groove 128 BPM and Techno 124 BPM tracks. Browse also shows
the unloaded 16-second replacement fixture, read from its cached analysis after
a restart. Unavailable previews show an em dash. No physical MIDI controller or
USB drive is attached; runtime statistics describe the container.

Beat FX picker images retain the `codex/ddj400-shift-fx-back` capture for the
catalogue/picker changes documented here. The picker and drawer include Day
captures; the primary Play/Browse/Settings images use Night mode.

See the [0.0.7 changelog](../CHANGELOG.md#007--unreleased) for the changes behind
these screens, or [return to the README](../README.md).

[Drawer](#controller-pad-drawer) | [Play](#play) | [Beat FX picker](#beat-fx-picker) | [Browse with previews](#browse-preview) | [General](#settings-general) | [Library](#settings-library) | [Pad FX](#settings-pad-fx) | [Device](#settings-device) | [Audio](#settings-audio) | [System](#settings-system) | [Info](#settings-info)

<a id="play"></a>

## Play

Two loaded decks with synchronized waveform type/palette rendering, deck overviews and the FX panel. The KEY and JUMP tabs share the right-hand panel.

![Play: Two loaded decks with synchronized waveform type/palette rendering, deck overviews and the FX panel. The KEY and JUMP tabs share the right-hand panel.](images/ui/0.0.7/play.png)

<a id="beat-fx-picker"></a>

## Beat FX picker

The Standard section follows the 25-name Rekordbox 7 single-mode Beat FX list.
Every entry is a documented [native approximation](BEAT_FX.md), with differences
from Rekordbox explained in the catalogue. Selecting an effect closes the picker
and leaves FX Off until activated. Page navigation and Close preserve selection.

Two columns provide 492px-wide effect buttons with at least 50px height and an
8px gutter. ECHO is selected here. The second page keeps the same row spacing.

![Standard Beat FX, Night mode, page 1 with ECHO selected.](images/ui/0.0.7/beat-fx-page-1.png)

![Standard Beat FX, Night mode, page 2.](images/ui/0.0.7/beat-fx-page-2.png)

<details>
<summary>Day mode and preserved Saved presets</summary>

![Standard Beat FX in Day mode.](images/ui/0.0.7/beat-fx-day.png)

Saved keeps existing preset files and identities. The former duplicate FILTER
labels now display as COLOR FILTER and RHYTHMIC FILTER.

![Saved presets, page 1, including COLOR FILTER.](images/ui/0.0.7/beat-fx-saved-1.png)

![Saved presets, page 2, including RHYTHMIC FILTER.](images/ui/0.0.7/beat-fx-saved-2.png)

</details>

<a id="browse-preview"></a>

## Browse with previews

Synthetic tracks in the compact library table, with cached waveform previews, including the unloaded 16-second replacement fixture, and both deck overviews visible below.

![Browse with previews: Synthetic tracks in the compact library table, with cached waveform previews, including the unloaded 16-second replacement fixture, and both deck overviews visible below.](images/ui/0.0.7/browse-preview.png)

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
header. Forward order is Hot Cues → Memory → Beat Jump → Pad FX → Beat Loop;
Previous reverses and wraps. Controller mode selection and touch share the same
per-deck display state. Performance pads are legends; Hot Cues and Memory retain
interactive cue pads. [Coordinates and mappings](../AGENTS.md#touch-drawer-navigation-and-padding-1024600).

![Beat Jump with separate previous and next buttons and balanced padding.](images/ui/0.0.7/controller-beat-jump.png)

![Beat Loop legend in the padded drawer.](images/ui/0.0.7/controller-beat-loop.png)

![Pad FX legend and touch mode navigation.](images/ui/0.0.7/controller-pad-fx.png)

![Memory cue pads with the same header and spacing.](images/ui/0.0.7/touch-memory.png)

![Pad FX drawer in Day mode.](images/ui/0.0.7/controller-pad-fx-day.png)

## Refreshing these images

Follow the [capture and review procedure](GUI_TESTING.md#documentation-screenshots)
and [agent screenshot policy](../AGENTS.md#published-ui-screenshots). Refresh
screens affected by UI changes in the same commit and link their sections from
the changelog. Raw captures remain ignored; only reviewed publication images
are stored here. Preserve this gallery once 0.0.7 is released.
