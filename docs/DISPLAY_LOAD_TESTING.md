# Two-deck display and replacement checks

Branch `codex/rekordbox-padfx-display`; eventual merge `codex/v0.0.7`.
Only the owned ARM64 container `bitedj-next-gui` is used.

## Behavior

Long deck titles and artist text scroll at 30 logical pixels per second after
a 1.5-second pause, pause at the end, then restart. Short labels remain static;
hidden labels stop their timers. The existing deck layout and Day/Night palette
remain authoritative.

Settings → General → Track Load exposes the native saved policy:

| Mode | Replacing a playing deck |
| --- | --- |
| Lock (default) | Reject the load. |
| Fader | Allow only with channel volume at zero or main output routing off; stop the replacement. |
| Stop | Allow replacement and stop, even if the request asks to play. |
| Live | Allow replacement and continue playing from the load point. |

Stopped decks retain ordinary load behavior. Preview and sampler behavior is
independent. Invalid settings default to Lock. The library, menus and drag
acceptance share the policy, with a final check before the player unloads the
current track. The custom Wayland drag overlay is preserved.

## Verification

Native tests cover elapsed scrolling and hidden timers, legacy policy migration,
invalid values, channel routing, and actual player replacement in all four modes.
Replacement tests wait for the expected engine track, rather than any loaded
track, to avoid observing the previous asynchronous load.

Review Play, Browse, Sampler, Levels and Settings at 1024×600 in Day mode; then
review a deliberately long title across multiple frames. Track Load controls
reserve 44px after the General row's padding and margin. This is a software
contrast/layout review, not a physical direct-sunlight measurement.

Evidence is stored under ignored `test-results/bitedj-next-gui/`. The existing
PAD FX validation remains documented in [PAD_FX_TESTING.md](PAD_FX_TESTING.md).

Verified on the owned ARM64 instance: 54 native regression tests passed, followed
by 20 focused tests after adding the rendered Day-color regression (55 distinct
tests). Day and Night screenshots show moving long titles, intact padding and
the four load-policy controls. PAD FX still fills its page without bottom
waveforms. Fader selection was checked and Lock restored for review.
