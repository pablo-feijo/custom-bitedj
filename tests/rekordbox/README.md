# Synthetic Rekordbox fixtures and live checks

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

These fixtures test [Custom Bite DJ](../../README.md), the independent BiteDJ fork.
The Rekordbox name identifies the export format under test, not sponsorship.

Generate all media locally with the Python standard library:

```sh
source ./scripts/test/gui-test-settings.sh
verify_test_instance_owner
python3 tests/rekordbox/make_fixture.py "$RESULTS_DIR/RekordboxFixture"
docker cp "$RESULTS_DIR/RekordboxFixture" "$CONTAINER_NAME:/media/"
```

The generator writes a DeviceSQL export and four original, synthetic 60-second
PCM tracks. No commercial tracks or proprietary user exports are bundled.
The first two tracks have DAT beat grids, PWV6/PWV7 display envelopes and PSSI
phrases. The third lacks EXT/2EX; the fourth has valid phrases/cues but damaged
waveform analysis. Load them through Browse → Rekordbox → RekordboxFixture to
exercise import. Loading WAV paths from the command line bypasses that path.

Cue positions on the exported tracks:

| Type | Position | Label | Loop end |
| --- | --- | --- | --- |
| Hot A | 4.1 s | A START | — |
| Hot B | 16.1 s | B CHORUS | — |
| Hot C | 32.1 s | C LOOP | 34.1 s |
| Hot D | 48.1 s | D OUTRO | — |
| Memory | 8.1 s | MEM 1 | — |
| Memory | 24.1 s | MEM 2 | — |
| Memory loop | 40.1 s | MEM LOOP | 44.1 s |
| Memory | 56.1 s | MEM 4 | — |

The missing-analysis fixture intentionally has no extended cues. The damaged
fixture must warn once and still allow audio and native waveform generation.

## Visual and timing procedure

1. Load the first two tracks from the export onto Deck 1/2. Inspect hot cues,
   memory cues and loops against the table above, including phrase boundaries.
2. Check RGB and 3 Band modes, linked/unlinked zoom, and Day/Night at 1024×600.
   Phrase and ruler strips should be discreet; cue targets must remain usable.
3. Toggle General → Phrases Off/On. Both overview and scrolling strips update;
   beats, cues and playback must not change. With both decks paused, stay on
   General and verify both bottom previews change immediately, without changing
   tabs, moving the playheads, or hovering over the previews. Check persistence
   after restart.
4. Play both decks. Capture audio from `auto_null.monitor` in the owned ARM64
   instance, and inspect RMS, peaks and underrun logs. Repeat with fallback tracks.
5. For timing, launch with `--developer --logLevel debug` and
   `BITEDJ_TEST_STATS_PATH=/tmp/rekordbox-stats.json`. Developer StatsManager
   exports cumulative import/render timings from its worker thread. Read the
   units field; compare two-deck render time with the configured frame budget.
   Collect a fresh run per mode/configuration; startup and load spikes must be
   distinguished from steady playback. Container results do not establish Pi
   hardware performance. Run timing checks without concurrent compilation.
6. Record exact branch, binary checksum, fixture, display mode and sampling
   interval locally. Leave the owned VNC running with the synthetic tracks for
   manual review; do not restart another branch's container.

Use `pkill -9 mixxx` only in the verified owned instance. Use `scrot -o` for a
fresh screenshot. All screenshots, generated exports/WAVs, benchmark JSON,
logs and native test XML belong under ignored `test-results/<instance>/`.
Keep generators, procedures and assertions in Git; **never commit run output**.
Native decoder/import fixtures live in `src/test/rekordboxdisplay_test.cpp` and
`src/test/rekordboxanlz_test.cpp`.

Compact overview cue labels retain the cue color as a small badge with contrasting text. Verify both hot-cue letters and memory numbers in Day/Night, with phrases enabled and disabled. Fixtures include distinct cue colors to make regressions visible.

Bottom-preview cue priority: use 2px colored marker lines with a contrasting border, painted above the countdown watermark. Keep cue letters/numbers and phrase labels at 8px. Verify hot cues and memories remain distinct with phrases On/Off and in Day/Night.

The main `cue_point` is shown as an orange **CUE** marker in both bottom previews, matching Play (`#ff6000`). It remains visible when the playhead is exactly on the cue. Verify this separately from hot-cue letters and memory-cue numbers, with phrases On/Off.

At overlapping positions, the orange main **CUE** line and label paint last, above hot cues and memory cues. Keep the CUE label unabridged; cue metadata and existing edit targets are unchanged. Test exact overlaps with a hot cue and a memory cue separately.

To reproduce exact overlap checks, pause Deck 1, click hot cue A in its overview, then press the deck CUE button to set the main cue at 4.1 seconds. On Deck 2, click memory 1 and press CUE to set it at 8.1 seconds. Confirm the orange CUE marker stays above both markers in the previews, then toggle phrases and Day/Night. Use only the generated fixture tracks for these edits.

## Large-library regression

Generate a new synthetic export with 10,000 tiny PCM files and deliberately
sparse playlist/folder sort positions:

```sh
python3 tests/rekordbox/make_large_fixture.py test-results/LargeRekordbox --tracks 10000
BITEDJ_TEST_USB_DIR="$PWD/test-results/LargeRekordbox" ./scripts/test/run-gui-test.sh
```

The generator refuses an existing output directory. Verify 10,000 device tracks,
one Stress Folder containing Stress Playlist, and 10,000 ordered playlist links.
Check `EXPLAIN QUERY PLAN` for `(rb_id, device)` lookups using the composite index.
Open Contents, sort BPM/key, change folders while loading, and switch tabs during
population. Painting/sorting must not add these files to the internal library.
Native BrowseThread tests also hold a WAL writer transaction while the worker
reads saved BPM/key and exercise a 1,000-track batch import without a per-file delay.

For conservative USB 2.0-era slow-media checks, compile the Linux-only test
interposer `tests/e2e/slow-storage.c` with the ARM64 builder (`cc -shared -fPIC
-O2 ... -o slow-storage.so -ldl`) and copy it into the owned GUI container.
Restart only that instance's Mixxx with `LD_PRELOAD=/tmp/slow-storage.so` and
`BITEDJ_SLOW_STORAGE_ROOT=/media/TestUSB`. The shim leaves data unchanged but
adds 2 ms per read/stat operation plus transfer time at 8 MiB/s. Verify its
operation counter in stderr, all 10,000 Rekordbox tracks and playlist links,
tab response while browsing Contents, and cancellation when changing folders.
This checks application behavior under slow I/O, not USB electrical/power,
controller, cable or drive compatibility; those still need physical hardware.
