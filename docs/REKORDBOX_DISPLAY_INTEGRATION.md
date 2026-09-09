# Rekordbox display integration

Target: two-deck BiteDJ, eventual merge into `codex/v0.0.7`.
Source: xsploit/bitedj `4c1dfec590f98851159fe7a64e3348e8aad306a5`.

## Stage 1: decoder contract (verified)

PWV6/PWV7 are mono display envelopes in mid/high/low byte order. Mirror
them into the two renderer channels; use a shared peak multiplier per
envelope, preserve relative bands, silence and the 150 Hz detail timebase.
Apply decoder offsets once and zero-pad beyond exported data instead of
stretching or holding the final column. Reject invalid dimensions and rates.

PSSI phrase positions use absolute one-based beat indices from the exported
variable-tempo grid. Decode masked and unmasked tags with the real Kaitai
parser. Unknown kinds remain unknown; invalid optional fills do not discard
valid boundaries. Only the final N+1 endpoint may use the final exported
beat's tempo. The import now invokes these helpers when loading from a Rekordbox device.

Native `RekordboxDisplayTest` covers band ordering, stereo mirroring, shared
normalization, silence, offsets, padding, malformed inputs, masked phrases,
variable tempo and final boundaries. It records decode time for a ten-minute
150 Hz envelope; a generous five-second timeout catches gross regressions,
not a claim about physical Pi performance.

## Stage 2: transactional import and alignment

- [x] Publish only a complete validated detail/overview pair; preserve native
  analysis when 2EX is missing, corrupt, duplicated or incomplete.
- [x] Reuse validated data by file identity, sample rate, duration and offset.
- [x] Avoid writing exported display envelopes into the native waveform cache.
- [x] Keep phrases separate from cues and retain an immutable exported grid
  for exact projection when the local beatgrid is edited or undone.
- [x] Protect already loaded tracks from import side effects.
- [x] Verify real DAT/EXT/2EX fixtures at the import boundary.

## Stage 3: render and benchmark

- [x] Preserve seconds per pixel at native 441 Hz and imported 150 Hz at every
  zoom level; retain existing stacked-band mode.
- [x] Render phrases beneath cue labels without altering cue hit targets.
- [x] Check two-deck Play and summary geometry at 1024×600 in Day/Night.
- [x] Measure frame time and import latency with two decks playing on ARM64.
- [x] Verify missing-analysis fallback and repeat native regressions.
- [ ] Complete a clean sustained audio run and investigate virtual-device underruns before closing the performance gate.

Do not import four-deck layouts, analysis-policy settings, streaming services
or appliance changes as part of this display integration.

Reproduce checks with [synthetic fixtures](../tests/rekordbox/README.md), including hot cues, memories and loops. Settings → General → Phrases toggles both main and overview strips and persists across restart. Overview annotations use letters/numbers only; full cue names remain in Play. Generated test results are deliberately excluded from source control.
