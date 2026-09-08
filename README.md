# BiteDJ — custom fork

A community customization of [Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj),
built on [Mixxx](https://mixxx.org/), for a Raspberry Pi touchscreen DJ appliance.
This is [pablo-feijo/custom-bitedj](https://github.com/pablo-feijo/custom-bitedj).
Our current development version is **0.0.7**, with a **1024×600** touch interface,
Sway/Wayland integration, USB-centered browsing, and custom DDJ-400 workflows.

BiteDJ is the foundation of this project. Credit belongs to
[Team Deckshark](https://github.com/TeamDeckshark),
[Alyxx](https://github.com/alyxxxinteractive), the other BiteDJ contributors,
and the [Mixxx developers](https://github.com/mixxxdj/mixxx).
Original copyright notices and contributor history remain in the repository.

## What this fork changes

Our custom work includes:

- Touchscreen track drag-and-drop adapted for Qt 6 on Wayland.
- Fullscreen service preferences and persistent screen rotation for the appliance.
- Raspberry Pi cursor workarounds and custom audio-device presentation.
- DDJ-400 Pad FX workflows and Shift + Filter control of the Beat FX Super parameter.
- Compact deck, sampler, settings, and waveform presentation refinements.

See the [change ledger](docs/DIFFS_FROM_BASE.md) for implementation details and
version history, and the [DDJ-400 mapping guide](docs/DDJ400_MAPPING.md) for controls.
The overview panel currently keeps **FX and KEY** tabs. Beat-typed effect timing
uses the skin’s native Beats parameter grid.

## Built on BiteDJ and Mixxx

The audio engine, mixing, cue/loop/sync/keylock machinery and controller framework
come from Mixxx and BiteDJ. BiteDJ adds the appliance interface, USB-based metadata
and analysis stores, device settings, notifications, and other standalone workflows.
These inherited capabilities are not new work by this fork.

The [preserved project description](docs/BITEDJ-UPSTREAM.md) retains the background
that previously occupied this README. The [upstream BiteDJ repository](https://github.com/TeamDeckshark/bitedj)
is the source for its current documentation and community development.

## Development and validation

This repository contains application source and the Raspberry Pi image workflow.
Application compilation and OS image generation are separate steps. The application
target is Linux; macOS can host the Docker build workflow.

- [Build and deploy](docs/BUILD_AND_DEPLOY.md): application build and deployment workflow.
- [Infrastructure](docs/INFRASTRUCTURE.md): appliance runtime and build architecture.
- [GUI and audio testing](docs/GUI_TESTING.md): local test environment and checks.
- [Networking and audio](docs/BiteDJ_Networking_Audio_Docs.md): configuration notes.

The repository build entry point for ARM64 is `./docker-build.sh --platform linux/arm64`.
OS image generation uses `./generate-pi-image.sh` and the `mixxx-pi-gen` checkout.
Follow the linked guides for prerequisites and deployment. Documentation and source
availability do not establish that a particular image or hardware configuration
has passed validation.

## Other forks and planned work

[xsploit’s PiFlex edition](https://github.com/xsploit/bitedj) is a related BiteDJ fork
with additional display, library, controller, Rekordbox, streaming, and download work.
Its approach to clear upstream attribution informed this README organization.

Our [fork review and feature map](docs/XSPLOIT_FORK_REVIEW.md) records the reviewed
revision, overlap with our code, candidate changes, and validation needed before
adoption. Entries are proposals; this documentation change does not implement them.

## Community, support, and credits

Special thanks to **ntamas94** and the contributors to
[Pioneered by ntamas](https://github.com/ntamas94/pioneered-by-ntamas) for the
skin inspiration behind our deck indicators, compact browsing, and waveform controls.

For issues specific to this custom fork, use
[this repository’s issue tracker](https://github.com/pablo-feijo/custom-bitedj/issues).
For the upstream community, see the
[Deckshark blog](https://www.deckshark.us/blogs/news) and
[BiteDJ Discord](https://discord.com/invite/WJw6vdZKwQ).

See [attribution](NOTICE.md), [LICENSE](LICENSE), and [COPYING](COPYING).
The main source is licensed under GPL-2.0-or-later as stated in `LICENSE`;
bundled components and skins retain their individual license notices.
