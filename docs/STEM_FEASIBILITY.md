# Stem feasibility for BiteDJ 0.0.6

<!-- Modified for Custom Bite DJ on 2026-09-09: clarify fork identity and attribution. -->

This guide describes [Custom Bite DJ](../README.md), an independent fork of
[Team Deckshark’s BiteDJ](https://github.com/TeamDeckshark/bitedj), based on Mixxx.

Assessment date: 2026-09-08. No stem engine changes applied.
The work remains a separate deferred engine project in the canonical
[roadmap](DISPLAY_AND_DECK_LAYOUT_PLAN.md#consolidated-follow-up-backlog).

BiteDJ declares Mixxx 2.5.6 as its base. The colored DRUMS/VOCAL/INST
controls in the reference skin request separately rendered files through its
external controller bridge; copying them would not add independent stem mixing.

Upstream's native implementation crosses the decoder, track metadata, reader
cache, time-stretchers, mixing engine, effects and skin controls. This is a
separate engine project rather than an XML addition:

- [Engine support #13070](https://github.com/mixxxdj/mixxx/pull/13070): 67 files,
  including cachingreaderworker, all three buffer scalers, enginebuffer,
  enginedeck, looping control and track metadata.
- [Stem controls #13086](https://github.com/mixxxdj/mixxx/pull/13086): 36 files,
  including deck processing, metadata and UI controls.
- [Control naming/follow-up #14244](https://github.com/mixxxdj/mixxx/pull/14244):
  eight files, including the deck engine.
- [Upstream feature scope](https://github.com/mixxxdj/mixxx/issues/13116) and
  [third-party file compatibility tests](https://github.com/mixxxdj/mixxx/issues/15304).

These changes overlap BiteDJ's USB read recovery, thread priorities, audio
routing resilience and waveform/loop customizations documented in
DIFFS_FROM_BASE.md. A selective backport still needs the multi-channel decode
and metadata prerequisites plus later correctness fixes. An upgrade requires
reconciling all BiteDJ engine divergences, not only the stem commits.

Recommendation: deliver source/beat-jump/status/INFO independently on this
branch. Scope a dedicated engine integration against a pinned upstream version,
with review of GPL compatibility and attribution for any reused code. No
upstream code has been copied into this feature implementation.

Before shipping stem buttons, the integration must pass ordinary stereo
regressions and two-deck pre-separated-file playback on the actual Pi. Verify
independent mute/volume, seek/loop/scratch continuity, keylock, headroom and USB
read recovery. Measure callback load/underruns with both decks and effects.
Metadata must determine stem names/order; map bass plus other parts explicitly
if offering the reference's three-button presentation. Live AI separation
remains outside this scope.

Hardware performance and stem audio continuity have not been tested in this
assessment. No functional stem controls are exposed in the skin yet.
