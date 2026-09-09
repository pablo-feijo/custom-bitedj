# BiteDJ control audit

Date: 2026-09-08. Scope: the current BiteDJ skin and its native widgets.

## Results

- 53 XML PushButton declarations exist; 51 are reachable from skin.xml through
  25 XML files. Every reachable declaration has a Connection. Literal keys pass
  syntax checks. Template-supplied keys require owner review, not string matching.
- `beffect.xml` and `templates/toggle_button.xml` are not instantiated by the
  current skin. The old beffect template contains a malformed effect-rack key;
  it is not a visible broken button. Keep it out of the live template tree.
- Run `python3 scripts/audit_skin_buttons.py` to repeat the reachability and
  connection checks. This checks structure, not live audio behavior.

## Backend ownership review

| Visible control family | Backend / connection owner | Verification |
| --- | --- | --- |
| Main tabs, settings tabs, FX/KEY/JUMP, cue drawer | Legacy skin parser and WidgetStack triggers | Source review; local navigation |
| Play, cue, quantize, keylock | EngineBuffer and engine controls | Existing engine tests; local transport |
| JUMP size/actions | LoopingControl | Local forward/back and size checks |
| KEY shifts/reset | KeyControl | Local +2/reset checks |
| Linked zoom | WLinkedZoom → BaseTrackPlayer zoom controls / WaveformWidgetFactory | Local synchronized zoom/reset |
| Per-deck time | Persisted skin controls → WNumberPos | Native independence test; local toggle/restart |
| FX routing, enable, mix, super, parameter grid | EffectRack / EffectUnit / EffectSlot and native parameter widgets | Source review; previous FX GUI/audio checks |
| Sampler pads, clone/load, unload, stop-all | Sampler engine controls / sampler settings | Connection-owner review; not a new exhaustive pad/DSP run |
| Cue bank / cue actions | Cue drawer triggers and native hot-cue widgets | Connection-owner review |
| Library column choices | LibraryColumnControl | Source review; existing settings controls |
| Cache/cue/metadata clear, played reset | Library action controls | Handler review; clears not executed during audit |
| Controller rescan/apply/revert | ControllerSettings | Handler review; physical controller checks deferred |
| Audio rescan/apply/revert and device rows | AudioDeviceSettings / WAudioDeviceList | Handler review; physical device checks deferred |
| USB rows, eject, power actions | SystemSettings / WUsbList | Handler review; power/eject not executed during audit |
| Wi-Fi/Bluetooth launch and Preferences | MixxxMainWindow | Handler review; Pi launch behavior deferred |
| Long titles | WLabel opt-in scrolling | Native full-text/geometry/render test; explicit title foreground |

Native widgets build some buttons in C++ rather than XML. Their handlers must
be reviewed separately; they are not counted in the 51 declarations above.
ON, source, key, loop size and INFO are readouts, not action buttons.
An empty FX slot has no audible effect to enable; it is not proof of a bad key.

## Hardware follow-up

Pi validation is deferred at the user's request. USB slot topology, thermal
sensor identity, sustained audio under load, physical touch spacing, and device
launch/eject behavior remain unchecked on hardware. Docker/VNC results do not
close these items. See PIONEERED_FEATURE_TODO.md.
