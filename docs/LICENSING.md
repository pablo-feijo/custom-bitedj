# Licensing and upstream attribution

Reviewed 2026-09-09 for **Custom Bite DJ**, the independent
[pablo-feijo/custom-bitedj fork](https://github.com/pablo-feijo/custom-bitedj).
The original BiteDJ project is [TeamDeckshark/bitedj](https://github.com/TeamDeckshark/bitedj),
based on [Mixxx](https://github.com/mixxxdj/mixxx). This fork is maintained
separately; project names and compatibility references do not claim endorsement.

## Custom Bite DJ policy

The main application and its Custom Bite DJ modifications remain
**GPL-2.0-or-later**. Fork modifications are Copyright (C) 2026 Pablo Feijo and
contributors; upstream authors retain their work and notices. The GPL permits
use, modification and redistribution, including commercial redistribution,
subject to its source-code and notice conditions. See the
[GNU GPLv2 FAQ](https://www.gnu.org/licenses/old-licenses/gpl-2.0-faq.html).

The project notice identifies this fork without changing the standard GPL text.
The skin retains GPLv3; the separate pi-gen keeps BSD-3-Clause; other components
retain their own licenses. Do not replace those terms with a blanket project
license or introduce noncommercial restrictions on inherited GPL code.

## License sources

| Material | Notice to retain | Scope |
| --- | --- | --- |
| Main application | [LICENSE](../LICENSE), [COPYING](../COPYING), source copyright headers | The upstream grant permits GPL version 2 or any later version. The GPL text and upstream authorship are preserved. |
| BiteDJ skin | [Skin LICENSE](../res/skins/BiteDJ/LICENSE) and [manifest](../res/skins/BiteDJ/skin.xml) | The dedicated license supplies GPLv3 and credits Deckshark and the Pioneered lineage. The generic `GPL` manifest label does not override it. |
| Selected PiFlex adaptations | [NOTICE](../NOTICE.md) and pinned source records in [the change ledger](DIFFS_FROM_BASE.md) | Preserve original notices alongside the adaptation credits. |
| Controller mappings | Individual source headers, including [DDJ-400](../res/controllers/Pioneer-DDJ-400-script.js) | Retain original mapping authors and reviewers as well as subsequent adaptation credits. |
| Bundled dependencies and fonts | Their local notices, including [PortAudio](../lib/portaudio/LICENSE.txt), [Kaitai](../lib/kaitai/LICENSE), [hidapi](../lib/hidapi/LICENSE.txt) and [OpenSans](../res/fonts/OpenSans.LICENSE.txt) | These examples are not a complete dependency license inventory. Do not replace their licenses with the application's label. |
| Image generator | [Pinned pi-gen LICENSE](https://github.com/pablo-feijo/bitedj-mixxx-pi-gen/blob/de8656e297f22d35428d6f06b987adf4ddb886bd/LICENSE) | BSD-3-Clause notice for the generator, including Raspberry Pi (Trading) Ltd.; application and OS packages keep their own terms. |

The application and skin license statements above are also present in the
[upstream application license](https://github.com/TeamDeckshark/bitedj/blob/main/LICENSE)
and [upstream skin license](https://github.com/TeamDeckshark/bitedj/blob/main/res/skins/BiteDJ/LICENSE),
checked on the review date. Local files at the distributed revision are the
record for that revision. This guide does not relicense third-party material.
Do not label an entire image or combined distribution “GPLv2-only.”

## Identity and historical material

Use **Custom Bite DJ** for this project's releases and documentation. Use
**BiteDJ by Team Deckshark** for the direct upstream, and credit **Mixxx** as its
foundation. [NOTICE](../NOTICE.md) records PiFlex, Pioneered, controller and
image-generator credits. Fork-specific issues belong in
[this repository's tracker](https://github.com/pablo-feijo/custom-bitedj/issues).

Technical names such as `BiteDJ`, `mixxx`, `us.deckshark.*`, skin paths, config
keys and screenshot labels identify inherited interfaces. Keeping them is not
an authorship claim. A blanket rename would break compatibility and erase
provenance. Names or logos in retained material do not establish permission to
market a product as endorsed by their owners; artwork and branding rights need
separate review for a distributed product.

[Preserved BiteDJ background](BITEDJ-UPSTREAM.md) keeps its inherited body and
credits under a provenance header. [Flatpak notes](../packaging/flatpak/README.md)
are identified as inherited Mixxx instructions. Bundled library Markdown remains
its authors' documentation. Dated development reports describe their recorded
state; they are not new upstream or hardware certification claims.

## Before distributing binaries or images

Attribution is only part of license compliance. For a release, verify the actual
artifact against its component licenses. The main GPL text's sections 1–3 cover
notice retention, changed-file notices and source distribution; the skin's GPLv3
sections 4–6 cover its corresponding requirements. Read the linked license texts
for the conditions and exceptions rather than treating this summary as a grant.

- Keep copyright notices, license texts and warranty disclaimers with the
  distributed components. Mark modified files with the required change notices
  and dates. A changelog or Git history alone is not a substitute for auditing
  those file-level requirements.
- Provide the matching corresponding source and build/install scripts through a
  method allowed by the applicable license. Record the exact application and
  submodule commits. A link to a moving default branch alone does not identify
  the source for a particular binary; GitHub source ZIPs also omit submodule contents.
- Preserve component notices in the assembled image and check its included
  packages, fonts, skins and other assets. Review GPLv3 installation-information
  obligations if distributing a covered user product, where applicable.
- Verify the installed distribution, not just the source checkout. CMake installs
  LICENSE, COPYING and NOTICE.md together. Confirm component-specific notices
  and corresponding-source delivery for the actual release artifact.

This documentation review establishes clearer attribution and records the
license notices found. It is not a complete source-header, dependency, artwork,
trademark or release-artifact compliance audit and does not certify legal compliance.
