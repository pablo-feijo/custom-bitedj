# Custom Bite DJ 0.0.8 UI gallery

Custom Bite DJ is an independent fork of Team Deckshark’s BiteDJ, based on Mixxx.
These 1024×600 Night-mode captures use synthetic Rekordbox fixtures and binary
`0.0.8-codex-v0-0-8-controller-validation.2` in an owned ARM64 test instance.
[Capture provenance](images/ui/0.0.8/capture-provenance.json) records the exact
build source snapshot and image hashes. [Capture procedure](GUI_TESTING.md#documentation-screenshots).
The released [0.0.7 gallery](UI_SCREENSHOTS.md) preserves the remaining unchanged pages.

## Play

Both decks start with Quantize enabled. Shift + jog adjusts the beat grid only
while the right-panel Grid tab is active. The loaded synthetic decks retain
exported Rekordbox RGB colors and phrase markers. RGB uses PWV4/PWV5 colors;
3 Band uses the independent PWV6/PWV7 band envelopes.

![Play with Grid and Quantize enabled](images/ui/0.0.8/play.png)

## Browse preview

The first two rows show exported overview waveforms without requiring native
analysis. Their loaded deck overviews keep the same exported colors. The last
two fixtures deliberately lack valid exports; dashes remain until a native
summary becomes available. Preview reads run on a bounded background worker and open filesystem caches read-only.

![Rekordbox browser waveform previews](images/ui/0.0.8/browse-preview.png)

## Settings General

Vinyl Brake Off, Short and Long retain their saved controls. Normal jog release
uses the selected native ramp; Long and Return to Play On are selected here.
The first page host is prepared during skin setup, and Return to Play reuses
an unchanged waveform layout after the loaded track’s widget updates.

![General settings with jog and Return to Play controls](images/ui/0.0.8/settings-general.png)

See [controller mappings](DDJ400_MAPPING.md), [release notes](../RELEASE_0.0.8.md)
and the [0.0.8 changelog](../CHANGELOG.md#008--unreleased).
