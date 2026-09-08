# Attribution

This custom BiteDJ fork is derived from
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), with work by
[Team Deckshark](https://github.com/TeamDeckshark),
[Alyxx](https://github.com/alyxxxinteractive), and other BiteDJ contributors.
BiteDJ is based on [Mixxx](https://github.com/mixxxdj/mixxx).
Original contributor history and copyright notices remain authoritative.

The README organization was informed by
[xsploit’s PiFlex fork](https://github.com/xsploit/bitedj/tree/4c1dfec590f98851159fe7a64e3348e8aad306a5),
particularly its distinction between fork additions, inherited capabilities,
and validation status. The first batch adapts xsploit’s programmatic FX enable fix (`4c1dfec590`),
library text-size persistence (`0144fc92b5`), header-order approach, and
Rekordbox page-chain/analysis error handling from the pinned revision above.
Our implementation retains the existing Wayland dragging and compact layout,
stores managed header state in settings, and uses external model row identities
for sorting. See [the batch record](docs/XSPLOIT_FIRST_BATCH.md) for scope and checks.
Original Mixxx and BiteDJ notices remain in the affected source. No PiFlex skin,
Pad FX architecture, companion service or OS image is imported by this batch.

The main source’s GPL-2.0-or-later terms are stated in [LICENSE](LICENSE).
See also [COPYING](COPYING); bundled libraries and skins retain their own notices.
Project and product names identify their respective projects and owners;
this custom fork does not imply their endorsement.
