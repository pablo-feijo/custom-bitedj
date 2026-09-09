# Effect catalogue review

Reviewed `codex/v0.0.7` at `559edc3def` and the DDJ-400 follow-up branch
`codex/ddj400-shift-fx-back`. Intended merge target: `codex/v0.0.7`.
This is a source/preset audit, not an audio-equivalence or hardware test.

## Findings before cleanup

1. **Two Filter labels describe different chains.** `12. FILTER` is the built-in
   bipolar Filter. `15. FILTER` is Tremolo followed by Filter, not a genuine
   filter-cutoff LFO. Rename these to **Filter** and **Rhythmic Filter** when
   implementing the catalogue cleanup; retain **Filter Echo** for the third,
   deliberately composite chain.
2. **Echo and Pingpong are exact preset duplicates.** `01_ECHO.xml` and
   `21_PINGPONG.xml` match after removing the name and XML whitespace, including
   all parameters and linking. Both set `pingpong_amount` to zero. Either give
   Pingpong distinct, verified stereo processing or remove it from the default list.
3. **Roll, Reverse Roll and Slip Roll are exact preset duplicates.** Files 06,
   07 and 10 all contain the same Echo configuration. There is no reversing or
   transport/slip operation in these XML presets. The separate Pad FX Roll uses
   native temporary loop-roll controls and must not be removed with these duplicates.
4. **Other names overstate what the chain contains.** Helix is Phaser; Spiral
   is an Echo variation; Overdrive is Bitcrusher. Treat these as approximations
   requiring accurate names and listening checks, not independent proprietary DSP.
5. **Preset updates do not migrate existing installations.**
   `importDefaultPresets()` copies a bundled file only if the same filename does
   not already exist in the settings directory. Renaming or deleting a bundled
   file alone can leave an old saved copy alongside the replacement. User presets
   and saved list membership also feed the lists. A cleanup needs a versioned
   migration that recognizes unchanged shipped presets and preserves user edits.
6. **Controller selection used the wrong control semantics.** The inherited
   mapping treats `chain_selector` as an absolute index and sends positive values
   for reverse selection. `EffectChain::slotControlChainPresetSelector()` interprets
   positive as next and negative as previous. The follow-up correction now sends
   +1/-1 and retains either-deck Shift handling; the regression asserts directions,
   not a simulated six-entry index. Native navigation has no six-entry cap.

## Bundled Beat FX inventory

There are 22 bundled presets. Shared backend IDs alone do not prove duplication;
the exact-duplicate findings above compare complete XML payloads without names.

| Preset | Nonempty processing slots |
| --- | --- |
| [1. ECHO](../res/effects/chains/01_ECHO.xml) | threebandbiquadeq → echo |
| [2. DELAY](../res/effects/chains/02_DELAY.xml) | threebandbiquadeq → echo |
| [3. REVERB](../res/effects/chains/03_REVERB.xml) | threebandbiquadeq → reverb |
| [4. FLANGER](../res/effects/chains/04_FLANGER.xml) | threebandbiquadeq → flanger |
| [5. TRANS](../res/effects/chains/05_TRANS.xml) | tremolo |
| [6. ROLL](../res/effects/chains/06_ROLL.xml) | echo |
| [7. REV ROLL](../res/effects/chains/07_REV_ROLL.xml) | echo |
| [8. HELIX](../res/effects/chains/08_HELIX.xml) | phaser |
| [9. SPIRAL](../res/effects/chains/09_SPIRAL.xml) | echo |
| [10. SLIP ROLL](../res/effects/chains/10_SLIP_ROLL.xml) | echo |
| [11. CRUSH](../res/effects/chains/11_CRUSH.xml) | bitcrusher |
| [12. FILTER](../res/effects/chains/12_FILTER.xml) | filter |
| [13. NOISE](../res/effects/chains/13_NOISE.xml) | whitenoise → filter |
| [14. ECHOVERB HP](../res/effects/chains/14_ECHOVERB_HP.xml) | threebandbiquadeq → echo → reverb |
| [15. FILTER](../res/effects/chains/15_FILTER_BEAT.xml) | tremolo → filter |
| [16. FILTER ECHO](../res/effects/chains/16_FILTER_ECHO.xml) | filter → echo |
| [17. MID-SIDE](../res/effects/chains/17_MID-SIDE.xml) | balance |
| [18. OVERDRIVE](../res/effects/chains/18_OVERDRIVE.xml) | bitcrusher |
| [19. PAN](../res/effects/chains/19_PAN.xml) | threebandbiquadeq → autopan |
| [20. PHASER](../res/effects/chains/20_PHASER.xml) | threebandbiquadeq → phaser |
| [21. PINGPONG](../res/effects/chains/21_PINGPONG.xml) | threebandbiquadeq → echo |
| [22. SIDE REVERB HP](../res/effects/chains/22_SIDE_REVERB_HP.xml) | threebandbiquadeq → reverb → balance |

## References from earlier tasks

- [TeamDeckshark base](https://github.com/TeamDeckshark/bitedj/tree/39d39434160150890c8276c7cde03e454218cb76/res/effects/chains):
  the locally reviewed base has five chain files; it is not the source of a
  validated 22-algorithm Pioneer catalogue.
- [xsploit/PiFlex](https://github.com/xsploit/bitedj/tree/4c1dfec590f98851159fe7a64e3348e8aad306a5/res/effects/chains):
  six bundled chain files at the previously reviewed commit, including a distinct
  `CFX Filter` using `us.bitedj.effects.cfxfilter` and a separate `Filter Echo`.
  Its [Pad FX guide](https://github.com/xsploit/bitedj/blob/4c1dfec590f98851159fe7a64e3348e8aad306a5/docs/pad-fx.md)
  distinguishes transport Roll, Filter LFO and private DSP lanes. This supports
  preserving our separate Pad FX catalogue and its stable saved IDs.
- [Pioneered skin audit](https://github.com/ntamas94/pioneered-by-ntamas/blob/main/docs/the-skin.md#what-was-taken-out-of-the-side-panel-and-why):
  the earlier reference removed effect UI whose controls did not match live
  processing. Its useful lesson here is to verify each visible label and control
  against the actual loaded backend/slot, rather than copy its layout or names.

## Recommended cleanup

- Define explicit Beat FX, Color/Quick FX and Pad FX catalogues. Shared DSP is
  legitimate; duplicate entries within one list need distinct purpose or removal.
- Keep named composite chains such as Filter Echo, Echoverb HP and Side Reverb HP.
- Consolidate exact duplicates and rename misleading presets before renumbering.
  Do not change Pad FX persisted IDs as part of list presentation cleanup.
- Use native relative navigation for the DDJ-400 so its list matches the screen.
- Add a migration for untouched shipped presets and saved list entries; retain
  customized presets. Test both clean and upgraded settings, including repeated
  startup, before publishing a replacement catalogue.
- Validate stereo Pingpong, roll/transport claims and every chain's visible
  parameter controls with synthetic audio. Refresh the Play screenshot and
  controller/list documentation when the visible catalogue changes.

The follow-up implementation is documented in [Beat FX](BEAT_FX.md). Its versioned
25-entry Standard section replaces these defaults in the primary picker while
preserving legacy/custom presets in Saved. The two old Filter labels have
separate display names; the new Ping Pong sets stereo feedback, and the three
Roll approximations have distinct native configurations and explicit limitations.
