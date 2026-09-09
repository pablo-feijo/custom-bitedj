# Play, waveform and Grid control maps

[Codex setup](../../../../docs/CODEX.md). Read this guide when its task trigger applies.

## Deck interaction additions

- Linked waveform zoom is below the two JUMP sections. Use native zoom controls
  and the waveform factory's synchronization; do not step both synchronized decks
  separately, which would apply each action twice.
- Deck time modes use persisted `[Skin],deck1_time_mode` and `deck2_time_mode`.
- For touch library loading, begin with a horizontal move to distinguish a drag
  from vertical scrolling. Only actual visible deck regions accept that drag;
  Escape cancels it. Highlight geometry must match the release target geometry.
- Overview waveform height must match its visible container (currently 38px).
  Keep the 8px minute ruler in a separate row; do not crop a double-height widget.
  Check RGB, FILT and 3 BAND after changing overview rendering. Stacked rendering
  uses bottom-origin image coordinates and must not get the symmetric translation.

## Cue rendering

Overview labels use hot-cue letters and memory numbers; full names remain in
the Play waveform. Phrase strips are 10px with 8px text; the overview ruler
is 8px with 6px text.

Compact overview cue labels retain the cue color as a small badge with contrasting text. Verify both hot-cue letters and memory numbers in Day/Night, with phrases enabled and disabled. Fixtures include distinct cue colors to make regressions visible.

Bottom-preview cue priority: use 2px colored marker lines with a contrasting border, painted above the countdown watermark. Keep cue letters/numbers and phrase labels at 8px. Verify hot cues and memories remain distinct with phrases On/Off and in Day/Night.

The main `cue_point` is shown as an orange **CUE** marker in both bottom previews, matching Play (`#ff6000`). It remains visible when the playhead is exactly on the cue. Verify this separately from hot-cue letters and memory-cue numbers, with phrases On/Off.

At overlapping positions, the orange main **CUE** line and label paint last, above hot cues and memory cues. Keep the CUE label unabridged; cue metadata and existing edit targets are unchanged. Test exact overlaps with a hot cue and a memory cue separately.

Waveform regressions: type and palette changes update Browse, Play and deck
summaries while paused. Do not restore filesystem access or `getTrack()` calls
in preview painting; cached summaries load on the background pool. Preserve
completion and waveform identity in the bounded pixmap cache.

## Right-panel Grid editor

Overview right-panel order is FX, Key, Jump, Grid. Grid appends index 3 to
`[FxPanel],current`; existing saved FX/Key/Jump indices stay unchanged.
Selecting `[FxPanel],grid` enables `gridEditMode` on both waveforms: beat lines
become fully opaque and waveform dragging positions the track. Leaving Grid
restores the configured beat-line opacity and normal seek-disabled interaction.
Any active waveform drag is released on a mode change or when its page hides.

Each deck has a separate 168px block: deck label, Earlier/Later, Set grid here,
then BPM −/+. All action buttons are 44px high with 4px spacing and 11px labels;
tab labels use 10px. The combined FX layout uses a 204px side panel; recheck Grid clearance.

| Label | `[ChannelN]` native control | Action |
| --- | --- | --- |
| Earlier | `beats_translate_earlier` | Shift grid earlier |
| Later | `beats_translate_later` | Shift grid later |
| Set grid here | `beats_translate_curpos` | Align nearest beat to current playhead |
| BPM − | `beats_adjust_slower` | Reduce grid BPM by 0.01 |
| BPM + | `beats_adjust_faster` | Increase grid BPM by 0.01 |

Buttons emit momentary press/release, with no background repeat timer. Actions
require a loaded track with an editable beat grid. BPM actions edit the track's
grid, not the playback-rate slider. No controller mapping changes are required.

Grid deck headers display `[ChannelN],file_bpm` to two decimal places so each
0.01 BPM adjustment is visible without changing the deck playback rate.

Combined 204px panel verified at 1024×600: Grid tab (994,70); deck 1
Earlier/Later (892/976,146), Set (934,194), BPM −/+ (892/976,242).
Deck 2 uses y=314/362/410. Touch targets and both-deck dragging passed.
The Grid panel and waveforms retain their geometry when the cue drawer opens.

## Key and shared panel controls

Tab centers: FX (850,70), Key (898,70), Jump (946,70), Grid (994,70).

The side panel preserves FX=0, Key=1, Jump=2 and Grid=3.
Key reads `visual_key`, uses ±2 semitone native commands, `sync_key` for harmonic
Match and `reset_key` for file-key reset. FX retains deck routing, enable and
wet/dry Mix, with a scrollable area for the first effect's loaded parameters.
Beat period buttons and controller Beat left/right share the native period and
range metadata. Pad mode labels are 12px; arrow touch targets remain 48×44px.

Key −2/+2 use
(873/970,158) and y=306; Match/Reset use (873/970,210) and y=358.
