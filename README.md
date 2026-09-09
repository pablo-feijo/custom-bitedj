# Custom Bite DJ

A two-deck DJ appliance for Raspberry Pi, with a **1024×600 touchscreen**,
USB-centered music browsing, and a custom **Pioneer DDJ-400** workflow.
Development targets **v0.0.7**.

## Features

- Compact deck displays, scrolling titles, and Day/Night modes.
- Touch-friendly browsing with saved column layouts and Wayland drag-and-drop.
- Configurable Pad FX with Normal/Shift banks, saved assignments, and independent effect lanes.
- Safer track replacement: **Lock / Fader / Stop / Live**.
- Rekordbox USB waveforms and optional phrase strips, with native analysis fallback.
- A saved Prepare queue and optional return to Play after loading.
- FX, key, beat-jump, linked zoom, and deck presentation controls.
- Appliance settings for audio, display rotation, devices, and system information.

See the [DDJ-400 guide](docs/DDJ400_MAPPING.md) for controls and the
[integration checklist](docs/XSPLOIT_NEXT_BATCH.md) for completed and planned work.

## Build and test

Build the ARM64 application with:

```sh
./docker-build.sh --platform linux/arm64
```

Launch an isolated local GUI/audio test instance with `./run-gui-test.sh`.
Each feature branch uses its own worktree, settings, build, and VNC ports.

- [Build and deploy](docs/BUILD_AND_DEPLOY.md)
- [GUI and audio testing](docs/GUI_TESTING.md)
- [Architecture](docs/INFRASTRUCTURE.md)
- [Change history](docs/DIFFS_FROM_BASE.md)

## Credits

Built on [BiteDJ by Team Deckshark](https://github.com/TeamDeckshark/bitedj)
and [Mixxx](https://github.com/mixxxdj/mixxx). Thank you to Team Deckshark,
[Alyxx](https://github.com/alyxxxinteractive), and their contributors for the foundation.

Special thanks to [xsploit](https://github.com/xsploit/bitedj) for the work behind
our adapted Pad FX, scrolling titles, safer loading, and Rekordbox improvements;
and to [ntamas94 and the Pioneered contributors](https://github.com/ntamas94/pioneered-by-ntamas)
for inspiring our deck indicators, browsing, and waveform controls.

Original notices and contributor history are preserved. See [NOTICE](NOTICE.md)
for attribution and [LICENSE](LICENSE) for GPL-2.0-or-later terms; bundled
components retain their own licenses.
