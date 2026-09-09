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
beat's tempo. These helpers are not yet enabled in track loading.

Native `RekordboxDisplayTest` covers band ordering, stereo mirroring, shared
normalization, silence, offsets, padding, malformed inputs, masked phrases,
variable tempo and final boundaries. It records decode time for a ten-minute
150 Hz envelope; a generous five-second timeout catches gross regressions,
not a claim about physical Pi performance. The ARM64 fixture run passed all
three decoder tests; the ten-minute envelope recorded under one millisecond
at timer resolution in the container.

## Stage 2: transactional import and alignment

- [ ] Publish only a complete validated detail/overview pair; preserve native
  analysis when 2EX is missing, corrupt, duplicated or incomplete.
- [ ] Reuse validated data by file identity, sample rate, duration and offset.
- [ ] Avoid writing exported display envelopes into the native waveform cache.
- [ ] Keep phrases separate from cues and retain an immutable exported grid
  for exact projection when the local beatgrid is edited or undone.
- [ ] Protect already loaded tracks from import side effects.
- [ ] Verify real DAT/EXT/2EX fixtures at the import boundary.

## Stage 3: render and benchmark

- [ ] Preserve seconds per pixel at native 441 Hz and imported 150 Hz at every
  zoom level; retain existing stacked-band mode.
- [ ] Render phrases beneath cue labels without altering cue hit targets.
- [ ] Check two-deck Play and summary geometry at 1024×600 in Day/Night.
- [ ] Measure frame time and import latency with two decks playing on ARM64.
- [ ] Verify missing-analysis fallback and repeat the native/audio regressions.

Do not import four-deck layouts, analysis-policy settings, streaming services
or appliance changes as part of this display integration.
