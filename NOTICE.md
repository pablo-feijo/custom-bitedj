# Custom Bite DJ — attribution and fork notice

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This custom BiteDJ fork is derived from
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), with work by
[Team Deckshark](https://github.com/TeamDeckshark),
[Alyxx](https://github.com/alyxxxinteractive), and other BiteDJ contributors.
BiteDJ is based on [Mixxx](https://github.com/mixxxdj/mixxx).
This independently maintained repository is
[pablo-feijo/custom-bitedj](https://github.com/pablo-feijo/custom-bitedj), not an
official Deckshark or Mixxx release. Fork-specific support belongs in this
repository's issue tracker. Original contributor history and copyright notices
remain authoritative; this notice does not replace them.

The README organization was informed by
[xsploit’s PiFlex fork](https://github.com/xsploit/bitedj/tree/4c1dfec590f98851159fe7a64e3348e8aad306a5),
particularly its distinction between fork additions, inherited capabilities,
and validation status. The first batch adapts xsploit’s programmatic FX enable fix (`4c1dfec590`),
library text-size persistence (`0144fc92b5`), header-order approach, and
Rekordbox page-chain/analysis error handling from the pinned revision above.
Our implementation retains the existing Wayland dragging and compact layout,
stores managed header state in settings, and uses external model row identities
for sorting. See [the change ledger](docs/DIFFS_FROM_BASE.md) for scope and checks.
Original Mixxx and BiteDJ notices remain in the affected source. That first
batch did not import the PiFlex skin, Pad FX architecture, companion service
or OS image; later Pad FX adaptations are credited below.

The main source’s GPL-2.0-or-later terms are stated in [LICENSE](LICENSE).
See also [COPYING](COPYING). The BiteDJ skin carries
[GPLv3 and separate copyright notices](res/skins/BiteDJ/LICENSE), including
Deckshark, Sven Boekelder, Bart van Oort, Clément Jonglez and bencejuhaaz.
Its notice records its origin in [Pioneered](https://github.com/timewasternl/Pioneered).
Bundled libraries, fonts and other components retain their own notices and terms.
See [licensing and distribution notes](docs/LICENSING.md).
Project and product names identify their respective projects and owners;
this custom fork does not imply their endorsement.

### Pad FX adaptation (integrated for 0.0.7; unreleased)

The Pad FX settings bridge, private effect lanes, Pad Echo DSP, controller
helper and regression fixtures adapt xsploit/bitedj revision
`4c1dfec590f98851159fe7a64e3348e8aad306a5`. The editor is reworked for 1024×600
with a single selected pad; the adapter targets DDJ-400 Normal/Shift MIDI and
preserves its shadow-note LED addresses. Original GPL licensing applies.

### Display and load-policy adaptation (integrated for 0.0.7; unreleased)

Elapsed-time scrolling labels and the shared deck-load policy adapt the same
pinned xsploit revision. This build retains two decks and defaults to Lock;
checks at the player boundary cover controller and clone requests as well as
UI loading. Rekordbox PWV6/PWV7 and PSSI decoder helpers and binary fixtures
also adapt that revision, together with validated import and phrase rendering.
Thank you to xsploit for sharing this work. Custom Bite DJ keeps its two-deck
layout, compact annotations, optional phrase visibility, and native fallback.

### Further presentation references

Deck indicators, browsing, waveform controls and system readouts were informed
by [Pioneered by ntamas](https://github.com/ntamas94/pioneered-by-ntamas) and the
Pioneered contributors. See the [implemented changes](docs/DIFFS_FROM_BASE.md).
This design credit is distinct from the inherited BiteDJ skin's Pioneered
lineage and does not relicense any copied material.

### Fork modification record

Custom changes are recorded in [CHANGELOG.md](CHANGELOG.md), the
[change ledger](docs/DIFFS_FROM_BASE.md), and Git history. Documentation and
attribution wording were revised for this fork on 2026-09-09. Existing copyright
holders, author credits and license texts are retained. References to hardware,
software names, logos or compatibility do not assert ownership or endorsement.

### Controller and image-generator lineage

The DDJ-400 script retains its Mixxx mapping authors Warker, nschloe, dj3730,
jusko and tiesjan, and reviewers Be-ing and Holzhaus in
[the source header](res/controllers/Pioneer-DDJ-400-script.js). Local and PiFlex
adaptations supplement that work; they do not replace those credits.

The separate image generator is a fork of
[fayaaz/mixxx-pi-gen](https://github.com/fayaaz/mixxx-pi-gen), built on
[Raspberry Pi's pi-gen](https://github.com/RPi-Distro/pi-gen).
Its [BSD-3-Clause license](https://github.com/pablo-feijo/bitedj-mixxx-pi-gen/blob/de8656e297f22d35428d6f06b987adf4ddb886bd/LICENSE)
and Raspberry Pi (Trading) Ltd. notice apply to that generator, not to every
package in the generated image.
