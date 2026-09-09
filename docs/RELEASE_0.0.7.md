# Custom Bite DJ 0.0.7

Release source: `codex/v0.0.7`. The pull request to `main` is merged by the
maintainer separately. The application reports `0.0.7`; the matching image is
named `bitedj-pi-v0.0.7`. Exact commits, hashes and build timestamps belong in
the artifact's provenance and checksums, not a moving branch label.

## Features

This release brings the compact two-deck touchscreen workflow, controller-synced
performance drawer, per-deck Grid/Key editing, Standard/Saved Beat FX, persistent
queues and visible-deck Auto Play together with Rekordbox waveform/phrase import.
System settings add touch clock/date editing, overclock recovery and separate
restart/power actions. See the [changes-only changelog](../CHANGELOG.md#007--2026-09-09),
[UI gallery](UI_SCREENSHOTS.md), [DDJ-400 mapping](DDJ400_MAPPING.md) and
[FX implementation notes](BEAT_FX.md).

## Removable storage

Discovery uses Linux mount metadata without querying a drive's filesystem
statistics. Folder enumeration and track metadata run off the GUI thread;
batches bound queued work and switching folders discards obsolete results.
Rekordbox indexes avoid repeated full-table lookups, and sparse/cyclic playlist
positions cannot grow traversal without bounds.

Regression coverage includes a synthetic 10,000-track export, sparse playlist
positions, a stalled row consumer, concurrent SQLite writes and slow reads at
8 MiB/s plus 2 ms per operation. These checks exercise application responsiveness,
not universal USB device compatibility. A Kingston DataTraveler connected at
480 Mb/s was also detected on a Raspberry Pi 4 and its 223 exported tracks imported. A track loaded and played from that
drive while Browse remained usable over an SSH-protected VNC test connection.
The original noisy Seagate HDD could not be validated reliably; software changes
do not establish that its power, cable or mechanics are healthy.

## Build and validation

Use the [build/deploy procedure](BUILD_AND_DEPLOY.md) and the exact pinned
`mixxx-pi-gen` commit. The image uses Debian 13 Trixie and the approved 0.0.7
boot defaults. A fresh image flash/boot is separate from installing and testing
the application on an existing Pi; do not claim one proves the other.

Run fast, native, removable and desktop E2E checks, verify a connected noVNC
desktop, and test the Pi before delivery. Preserve test results beside release
artifacts. Physical MIDI gestures, sustained audio/thermal performance and
arbitrary USB devices remain separate hardware acceptance checks.

After validation, remove temporary Pi test profiles/media and demo playlists
without deleting music referenced by those playlists. Keep the previous release
artifact until the replacement binary and image hashes and embedded versions
have passed verification.
