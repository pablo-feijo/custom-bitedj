// Pioneer-DDJ-400-script.js
// ****************************************************************************
// * Mixxx mapping script file for the Pioneer DDJ-400.
// * Authors: Warker, nschloe, dj3730, jusko, tiesjan
// * Reviewers: Be-ing, Holzhaus
// * Manual: https://manual.mixxx.org/2.3/en/hardware/controllers/pioneer_ddj_400.html
// ****************************************************************************
//
//  Implemented (as per manufacturer's manual):
//      * Mixer Section (Faders, EQ, Filter, Gain, Cue)
//      * Browsing and loading + Waveform zoom (shift)
//      * Jogwheels, Scratching, Bending, Loop adjust
//      * Cycle Temporange
//      * Beat Sync
//      * Hot Cue Mode
//      * Beat Loop Mode
//      * Beat Jump Mode
//      * Sampler Mode
//
//  Custom (Mixxx specific mappings):
//      * BeatFX: Assigned Effect Unit 1
//                < LEFT toggles focus between Effects 1, 2 and 3 leftward
//                > RIGHT toggles focus between Effects 1, 2 and 3 rightward
//                v DOWN loads next effect entry for focused Effect
//                SHIFT + v UP loads previous effect entry for focused Effect
//                LEVEL/DEPTH controls the Mix knob of the Effect Unit
//                SHIFT + LEVEL/DEPTH controls the Meta knob of the focused Effect
//                ON/OFF toggles focused effect slot
//                SHIFT + ON/OFF disables all three effect slots.
//      * Memory Cue CALL, MEMORY and DELETE (CUE/LOOP CALL arrows)
//      * Toggle quantize (Shift + channel cue)
//
//  Not implemented (after discussion and trial attempts):
//      * Loop Section:
//        * -4BEAT auto loop (hacky---prefer a clean way to set a 4 beat loop
//                            from a previous position on long press)
//
//      * Secondary pad modes (trial attempts complex and too experimental)
//        * Keyboard mode
//        * Pad FX1
//        * Pad FX2
//        * Keyshift mode

var PioneerDDJ400 = {};

PioneerDDJ400.lights = {
    beatFx: {
        status: 0x94,
        data1: 0x47,
    },
    shiftBeatFx: {
        status: 0x94,
        data1: 0x43,
    },
    deck1: {
        vuMeter: {
            status: 0xB0,
            data1: 0x02,
        },
        playPause: {
            status: 0x90,
            data1: 0x0B,
        },
        shiftPlayPause: {
            status: 0x90,
            data1: 0x47,
        },
        cue: {
            status: 0x90,
            data1: 0x0C,
        },
        shiftCue: {
            status: 0x90,
            data1: 0x48,
        },
    },
    deck2: {
        vuMeter: {
            status: 0xB0,
            data1: 0x02,
        },
        playPause: {
            status: 0x91,
            data1: 0x0B,
        },
        shiftPlayPause: {
            status: 0x91,
            data1: 0x47,
        },
        cue: {
            status: 0x91,
            data1: 0x0C,
        },
        shiftCue: {
            status: 0x91,
            data1: 0x48,
        },
    },
};

// Store timer IDs
PioneerDDJ400.timers = {};

// Jog wheel constants
PioneerDDJ400.vinylMode = true;
PioneerDDJ400.alpha = 1.0/8;
PioneerDDJ400.beta = PioneerDDJ400.alpha/32;

// Multiplier for fast seek through track using SHIFT+JOGWHEEL
PioneerDDJ400.fastSeekScale = 150;
PioneerDDJ400.bendScale = 0.8;

PioneerDDJ400.tempoRanges = [0.06, 0.10, 0.25, 0.50, 1.00];

PioneerDDJ400.shiftButtonDown = [false, false];

// Jog wheel loop adjust
PioneerDDJ400.loopAdjustIn = [false, false];
PioneerDDJ400.loopAdjustOut = [false, false];
PioneerDDJ400.loopAdjustMultiply = 50;

// Beatjump pad (beatjump_size values)
PioneerDDJ400.beatjumpSizeForPad = {
    0x20: -1, // PAD 1
    0x21: 1,  // PAD 2
    0x22: -2, // PAD 3
    0x23: 2,  // PAD 4
    0x24: -4, // PAD 5
    0x25: 4,  // PAD 6
    0x26: -8, // PAD 7
    0x27: 8   // PAD 8
};

PioneerDDJ400.quickJumpSize = 32;

// Used for tempo slider
PioneerDDJ400.highResMSB = {
    "[Channel1]": {},
    "[Channel2]": {}
};

PioneerDDJ400.trackLoadedLED = function(value, group, _control) {
    midi.sendShortMsg(
        0x9F,
        group.match(script.channelRegEx)[1] - 1,
        value > 0 ? 0x7F : 0x00
    );
};

PioneerDDJ400.toggleLight = function(midiIn, active) {
    midi.sendShortMsg(midiIn.status, midiIn.data1, active ? 0x7F : 0);
};

//
// Init
//

PioneerDDJ400.init = function() {
    engine.setValue("[EffectRack1_EffectUnit1]", "show_focus", 1);

    engine.makeUnbufferedConnection("[Channel1]", "vu_meter", PioneerDDJ400.vuMeterUpdate);
    engine.makeUnbufferedConnection("[Channel2]", "vu_meter", PioneerDDJ400.vuMeterUpdate);

    PioneerDDJ400.toggleLight(PioneerDDJ400.lights.deck1.vuMeter, false);
    PioneerDDJ400.toggleLight(PioneerDDJ400.lights.deck2.vuMeter, false);

    engine.softTakeover("[Channel1]", "rate", true);
    engine.softTakeover("[Channel2]", "rate", true);
    engine.softTakeover("[EffectRack1_EffectUnit1_Effect1]", "meta", true);
    engine.softTakeover("[EffectRack1_EffectUnit1_Effect2]", "meta", true);
    engine.softTakeover("[EffectRack1_EffectUnit1_Effect3]", "meta", true);
    engine.softTakeover("[EffectRack1_EffectUnit1]", "mix", true);

    const samplerCount = 16;
    if (engine.getValue("[App]", "num_samplers") < samplerCount) {
        engine.setValue("[App]", "num_samplers", samplerCount);
    }
    for (let i = 1; i <= samplerCount; ++i) {
        engine.makeConnection("[Sampler" + i + "]", "play", PioneerDDJ400.samplerPlayOutputCallbackFunction);
    }

    engine.makeConnection("[Channel1]", "track_loaded", PioneerDDJ400.trackLoadedLED);
    engine.makeConnection("[Channel2]", "track_loaded", PioneerDDJ400.trackLoadedLED);

    // play the "track loaded" animation on both decks at startup
    midi.sendShortMsg(0x9F, 0x00, 0x7F);
    midi.sendShortMsg(0x9F, 0x01, 0x7F);

    PioneerDDJ400.setLoopButtonLights(0x90, 0x7F);
    PioneerDDJ400.setLoopButtonLights(0x91, 0x7F);

    engine.makeConnection("[Channel1]", "loop_enabled", PioneerDDJ400.loopToggle);
    engine.makeConnection("[Channel2]", "loop_enabled", PioneerDDJ400.loopToggle);

    engine.makeConnection("[Channel1]", "loop_start_position", PioneerDDJ400.loopInPending);
    engine.makeConnection("[Channel2]", "loop_start_position", PioneerDDJ400.loopInPending);
    engine.makeConnection("[Channel1]", "loop_end_position", PioneerDDJ400.loopInPending);
    engine.makeConnection("[Channel2]", "loop_end_position", PioneerDDJ400.loopInPending);

    for (let i = 1; i <= 3; i++) {
        engine.makeConnection("[EffectRack1_EffectUnit1_Effect" + i +"]", "enabled", PioneerDDJ400.toggleFxLight);
    }

    // Bite DJ: jog mode is chosen in the in-skin General settings tab via the
    // [BiteDJ],vinyl_mode CO (1 = Vinyl/scratch, 0 = CDJ/pitch-bend). Subscribe
    // so the choice applies live, and trigger() once to seed the current value.
    // On an unpatched Mixxx the CO does not exist, makeConnection returns nothing,
    // and we keep the hard-coded default above.
    const vinylModeConnection = engine.makeConnection(
        "[BiteDJ]", "vinyl_mode", PioneerDDJ400.setVinylMode);
    if (vinylModeConnection) {
        vinylModeConnection.trigger();
    }

    // query the controller for current control positions on startup
    midi.sendSysexMsg([0xF0, 0x00, 0x40, 0x05, 0x00, 0x00, 0x02, 0x06, 0x00, 0x03, 0x01, 0xf7], 12);
};

//
// Waveform zoom
//

PioneerDDJ400.waveformZoom = function(midichan, control, value, status, group) {
    if (value === 0x7f) {
        script.triggerControl(group, "waveform_zoom_up", 100);
    } else {
        script.triggerControl(group, "waveform_zoom_down", 100);
    }
};

// BROWSE rotate: zoom the waveform while the play screen ([Tab],current == 0)
// is active, otherwise scroll the library as usual.
PioneerDDJ400.loadDoubleTapTime = {
    "[Channel1]": 0,
    "[Channel2]": 0
};

PioneerDDJ400.loadTrack = function(channel, control, value, status, group) {
    if (value === 0x7F) {
        var now = Date.now();
        var last = PioneerDDJ400.loadDoubleTapTime[group] || 0;
        PioneerDDJ400.loadDoubleTapTime[group] = now;
        
        if (now - last < 500) {
            var otherDeck = (group === "[Channel1]") ? 2 : 1;
            var otherGroup = (group === "[Channel1]") ? "[Channel2]" : "[Channel1]";
            
            engine.setValue(group, "CloneFromDeck", otherDeck);
            
            // Mixxx often aborts CloneFromDeck if the track is already loaded by the first tap.
            // We forcefully copy position, sync, and play state to guarantee Instant Doubles.
            var otherPos = engine.getValue(otherGroup, "playposition");
            engine.setValue(group, "playposition", otherPos);
            
            if (engine.getValue(otherGroup, "sync_enabled")) {
                engine.setValue(group, "sync_enabled", 1);
            }
            
            engine.setValue(group, "beatsync_phase", 1);
            engine.setValue(group, "beatsync_phase", 0);
            
            var isPlaying = engine.getValue(otherGroup, "play");
            engine.setValue(group, "play", isPlaying);
        } else {
            engine.setValue(group, "LoadSelectedTrack", 1);
        }
    }
};

PioneerDDJ400.browseRotate = function(midichan, control, value, status) {
    if (engine.getValue("[Tab]", "current") === 0) {
        PioneerDDJ400.waveformZoom(midichan, control, value, status, "[Channel1]");
    } else {
        engine.setValue("[Library]", "MoveVertical", value > 0x40 ? value - 0x80 : value);
    }
};

PioneerDDJ400.browseShiftPress = function(channel, control, value, status, group) {
    if (value === 0x7F) {
        var currentTab = engine.getValue("[Tab]", "current");
        if (currentTab !== 1) {
            engine.setValue("[Tab]", "current", 1);
        } else {
            engine.setValue("[Tab]", "current", 0);
        }
    }
};

// From Play, BROWSE press opens the browser. On the browser and other screens
// it retains the mapping's existing focus-forward behavior.
PioneerDDJ400.browsePress = function(_midichan, _control, value) {
    if (value === 0) {
        return;
    }
    if (engine.getValue("[Tab]", "current") === 0) {
        engine.setValue("[Tab]", "current", 1);
        engine.setValue("[Tab]", "library", 1);
        return;
    }
    script.triggerControl("[Library]", "MoveFocusForward", 100);
};

// SHIFT + right LOAD toggles the Browser.
PioneerDDJ400.toggleBrowser = function(_midichan, _control, value) {
    if (value === 0) {
        return;
    }
    const browserOpen = engine.getValue("[Tab]", "current") === 1;
    engine.setValue("[Tab]", "current", browserOpen ? 0 : 1);
    engine.setValue("[Tab]", browserOpen ? "overview" : "library", 1);
};

//
// Channel level lights
//

PioneerDDJ400.vuMeterUpdate = function(value, group) {
    const newVal = value * 150;

    switch (group) {
    case "[Channel1]":
        midi.sendShortMsg(0xB0, 0x02, newVal);
        break;

    case "[Channel2]":
        midi.sendShortMsg(0xB1, 0x02, newVal);
        break;
    }
};

//
// Effects
//

PioneerDDJ400.toggleFxLight = function(_value, _group, _control) {
    const enabled = engine.getValue(PioneerDDJ400.focusedFxGroup(), "enabled");

    PioneerDDJ400.toggleLight(PioneerDDJ400.lights.beatFx, enabled);
    PioneerDDJ400.toggleLight(PioneerDDJ400.lights.shiftBeatFx, enabled);
};

// Bite DJ skin only renders one effect slot (Effect1), so all BEAT FX
// controls operate on it directly rather than on Mixxx's per-chain
// "focused_effect" slot-cycling mechanism.
PioneerDDJ400.focusedFxGroup = function() {
    return "[EffectRack1_EffectUnit1_Effect1]";
};

PioneerDDJ400.beatFxLevelDepthRotate = function(_channel, _control, value) {
    // Ignore physical knob if Pad FX is currently being held!
    if (PioneerDDJ400.padFxActiveCount > 0) return;

    // Map Level/Depth to Beat FX MIX knob (both with and without Shift)
    // so Shift + Filter (Super) and Level/Depth (Mix) can be tweaked simultaneously.
    engine.setValue("[EffectRack1_EffectUnit1]", "mix", value / 0x7F);
};

// Bite DJ skin only renders one effect slot (Effect1), so the BEAT
// LEFT/RIGHT buttons step through the on-screen bucket grid of the
// loaded Beats-typed parameter instead of switching focused slot.
// Order matches the row template's reading order
// (⅛ → ¼ → ½ → 1 → 2 → 4), values are raw rate-in-cycles-per-beat.
PioneerDDJ400.beatFxBuckets = [8, 4, 2, 1, 0.5, 0.25];

PioneerDDJ400.findBeatsParameter = function(group) {
    for (let i = 1; i <= 16; i++) {
        if (engine.getValue(group, "parameter" + i + "_loaded") !== 1) {
            continue;
        }
        if (engine.getValue(group, "parameter" + i + "_units") === 1) {
            return i;
        }
    }
    return -1;
};

PioneerDDJ400.stepBeatFxBucket = function(direction) {
    const group = "[EffectRack1_EffectUnit1_Effect1]";
    const paramIndex = PioneerDDJ400.findBeatsParameter(group);
    if (paramIndex === -1) { return; }

    const buckets = PioneerDDJ400.beatFxBuckets;
    const valueKey = "parameter" + paramIndex + "_value";
    const current = engine.getValue(group, valueKey);

    // Snap to nearest bucket, then step. Off-bucket values (rare —
    // bucket presses are the only writes — but possible via a MIDI
    // mapping that pokes a raw value) round to the closest match.
    let closest = 0;
    let bestDist = Math.abs(buckets[0] - current);
    for (let i = 1; i < buckets.length; i++) {
        const dist = Math.abs(buckets[i] - current);
        if (dist < bestDist) {
            bestDist = dist;
            closest = i;
        }
    }

    let next = closest + direction;
    if (next < 0) { next = 0; }
    if (next >= buckets.length) { next = buckets.length - 1; }
    if (next === closest) { return; }

    engine.setValue(group, valueKey, buckets[next]);
};

PioneerDDJ400.beatFxLeftPressed = function(_channel, _control, value) {
    if (value === 0) { return; }

    PioneerDDJ400.stepBeatFxBucket(-1);
};

PioneerDDJ400.beatFxRightPressed = function(_channel, _control, value) {
    if (value === 0) { return; }

    PioneerDDJ400.stepBeatFxBucket(1);
};

PioneerDDJ400.beatFxSelectPressed = function(_channel, _control, value) {
    if (value === 0) { return; }

    var unitGroup = "[EffectRack1_EffectUnit1]";
    var current = engine.getValue(unitGroup, "chain_selector");
    
    // The top 6 effects are our custom common ones (1-indexed in UI, but maybe 1-indexed in chain_selector?)
    // In Mixxx, chain_selector is 1-indexed or 0-indexed? It is 1-indexed (0 means empty/none in some versions, but 1 is first chain).
    // Let's cycle 1 -> 6
    if (current >= 6 || current < 1) {
        engine.setValue(unitGroup, "chain_selector", 1);
    } else {
        engine.setValue(unitGroup, "chain_selector", current + 1);
    }
};

PioneerDDJ400.beatFxSelectShiftPressed = function(_channel, _control, value) {
    if (value === 0) { return; }

    var unitGroup = "[EffectRack1_EffectUnit1]";
    var current = engine.getValue(unitGroup, "chain_selector");
    
    if (current <= 1 || current > 6) {
        engine.setValue(unitGroup, "chain_selector", 6);
    } else {
        engine.setValue(unitGroup, "chain_selector", current - 1);
    }
};

PioneerDDJ400.beatFxOnOffPressed = function(_channel, _control, value) {
    if (value === 0) { return; }

    const toggleEnabled = !engine.getValue(PioneerDDJ400.focusedFxGroup(), "enabled");
    engine.setValue(PioneerDDJ400.focusedFxGroup(), "enabled", toggleEnabled);

    // DEBUG LOGGING
    var ch1 = engine.getValue("[EffectRack1_EffectUnit1]", "group_[Channel1]_enable");
    var ch2 = engine.getValue("[EffectRack1_EffectUnit1]", "group_[Channel2]_enable");
    var master = engine.getValue("[EffectRack1_EffectUnit1]", "group_[Master]_enable");
    var mix = engine.getValue("[EffectRack1_EffectUnit1]", "mix");
    var meta = engine.getValue(PioneerDDJ400.focusedFxGroup(), "meta");
    var enabled = engine.getValue(PioneerDDJ400.focusedFxGroup(), "enabled");
    
    print("DEBUG_FX: CH1=" + ch1 + " CH2=" + ch2 + " MASTER=" + master + " MIX=" + mix + " META=" + meta + " ENABLED=" + enabled);
};

PioneerDDJ400.beatFxOnOffShiftPressed = function(_channel, _control, value) {
    if (value === 0) { return; }

    engine.setParameter("[EffectRack1_EffectUnit1]", "mix", 0);
    engine.softTakeoverIgnoreNextValue("[EffectRack1_EffectUnit1]", "mix");

    for (let i = 1; i <= 3; i++) {
        engine.setValue("[EffectRack1_EffectUnit1_Effect" + i + "]", "enabled", 0);
    }
    PioneerDDJ400.toggleLight(PioneerDDJ400.lights.beatFx, false);
    PioneerDDJ400.toggleLight(PioneerDDJ400.lights.shiftBeatFx, false);
};

PioneerDDJ400.beatFxChannel = function(_channel, control, value, _status, group) {
    if (value === 0x00) { return; }

    // MASTER routes the FX to both decks so that both DECK ASSIGN buttons in the
    // FX pane stay highlighted. We enable the individual channels rather than
    // group_[Master]_enable so the on-screen buttons (bound to the per-channel
    // enables) reflect the selection.
    const master = control === 0x14,
        enableChannel1 = control === 0x10 || master ? 1 : 0,
        enableChannel2 = control === 0x11 || master ? 1 : 0;

    engine.setValue(group, "group_[Channel1]_enable", enableChannel1);
    engine.setValue(group, "group_[Channel2]_enable", enableChannel2);
    engine.setValue(group, "group_[Master]_enable", 0);
};

//
// Loop IN/OUT ADJUST
//

PioneerDDJ400.toggleLoopAdjustIn = function(channel, _control, value, _status, group) {
    if (value === 0 || engine.getValue(group, "loop_enabled" === 0)) {
        return;
    }
    PioneerDDJ400.loopAdjustIn[channel] = !PioneerDDJ400.loopAdjustIn[channel];
    PioneerDDJ400.loopAdjustOut[channel] = false;
};

PioneerDDJ400.toggleLoopAdjustOut = function(channel, _control, value, _status, group) {
    if (value === 0 || engine.getValue(group, "loop_enabled" === 0)) {
        return;
    }
    PioneerDDJ400.loopAdjustOut[channel] = !PioneerDDJ400.loopAdjustOut[channel];
    PioneerDDJ400.loopAdjustIn[channel] = false;
};

// Two signals are sent here so that the light stays lit/unlit in its shift state too
PioneerDDJ400.setReloopLight = function(status, value) {
    midi.sendShortMsg(status, 0x4D, value);
    midi.sendShortMsg(status, 0x50, value);
};


PioneerDDJ400.setLoopButtonLights = function(status, value) {
    [0x10, 0x11, 0x4E, 0x4C].forEach(function(control) {
        midi.sendShortMsg(status, control, value);
    });
};

PioneerDDJ400.startLoopLightsBlink = function(channel, control, status, group) {
    let blink = 0x7F;

    PioneerDDJ400.stopLoopLightsBlink(group, control, status);

    PioneerDDJ400.timers[group][control] = engine.beginTimer(500, () => {
        blink = 0x7F - blink;

        // When adjusting the loop out position, turn the loop in light off
        if (PioneerDDJ400.loopAdjustOut[channel]) {
            midi.sendShortMsg(status, 0x10, 0x00);
            midi.sendShortMsg(status, 0x4C, 0x00);
        } else {
            midi.sendShortMsg(status, 0x10, blink);
            midi.sendShortMsg(status, 0x4C, blink);
        }

        // When adjusting the loop in position, turn the loop out light off
        if (PioneerDDJ400.loopAdjustIn[channel]) {
            midi.sendShortMsg(status, 0x11, 0x00);
            midi.sendShortMsg(status, 0x4E, 0x00);
        } else {
            midi.sendShortMsg(status, 0x11, blink);
            midi.sendShortMsg(status, 0x4E, blink);
        }
    });

};

PioneerDDJ400.stopLoopLightsBlink = function(group, control, status) {
    PioneerDDJ400.timers[group] = PioneerDDJ400.timers[group] || {};

    if (PioneerDDJ400.timers[group][control] !== undefined) {
        engine.stopTimer(PioneerDDJ400.timers[group][control]);
    }
    PioneerDDJ400.timers[group][control] = undefined;
    PioneerDDJ400.setLoopButtonLights(status, 0x7F);
};

PioneerDDJ400.loopToggle = function(value, group, control) {
    const status = group === "[Channel1]" ? 0x90 : 0x91,
        channel = group === "[Channel1]" ? 0 : 1;

    PioneerDDJ400.setReloopLight(status, value ? 0x7F : 0x00);

    if (value) {
        PioneerDDJ400.stopLoopInPendingBlink(status, group);
        PioneerDDJ400.startLoopLightsBlink(channel, control, status, group);
    } else {
        PioneerDDJ400.stopLoopLightsBlink(group, control, status);
        PioneerDDJ400.loopAdjustIn[channel] = false;
        PioneerDDJ400.loopAdjustOut[channel] = false;
    }
};

// loop_enabled stays 0 until OUT is pressed, so we watch the position COs.
PioneerDDJ400.loopInPending = function(_value, group) {
    const status = group === "[Channel1]" ? 0x90 : 0x91;

    if (engine.getValue(group, "loop_enabled") > 0) {
        return;
    }

    const inSet = engine.getValue(group, "loop_start_position") >= 0;
    const outSet = engine.getValue(group, "loop_end_position") >= 0;

    if (inSet && !outSet) {
        PioneerDDJ400.startLoopInPendingBlink(status, group);
    } else {
        PioneerDDJ400.stopLoopInPendingBlink(status, group);
    }
};

PioneerDDJ400.startLoopInPendingBlink = function(status, group) {
    PioneerDDJ400.stopLoopInPendingBlink(status, group);

    let blink = 0x7F;
    PioneerDDJ400.timers[group] = PioneerDDJ400.timers[group] || {};
    PioneerDDJ400.timers[group]["loopInPending"] = engine.beginTimer(500, () => {
        blink = 0x7F - blink;
        midi.sendShortMsg(status, 0x10, blink);
        midi.sendShortMsg(status, 0x4C, blink);
    });
};

PioneerDDJ400.stopLoopInPendingBlink = function(status, group) {
    PioneerDDJ400.timers[group] = PioneerDDJ400.timers[group] || {};
    if (PioneerDDJ400.timers[group]["loopInPending"] !== undefined) {
        engine.stopTimer(PioneerDDJ400.timers[group]["loopInPending"]);
        PioneerDDJ400.timers[group]["loopInPending"] = undefined;
        midi.sendShortMsg(status, 0x10, 0x7F);
        midi.sendShortMsg(status, 0x4C, 0x7F);
    }
};

//
// CUE/LOOP CALL
//

PioneerDDJ400.cueLoopCallLeft = function(_channel, _control, value, _status, group) {
    if (value) {
        if (engine.getValue(group, "loop_enabled") > 0) {
            engine.setValue(group, "loop_scale", 0.5);
        } else {
            engine.setValue(group, "memorycue_prev", 1);
        }
    }
};

PioneerDDJ400.cueLoopCallRight = function(_channel, _control, value, _status, group) {
    if (value) {
        if (engine.getValue(group, "loop_enabled") > 0) {
            engine.setValue(group, "loop_scale", 2.0);
        } else {
            engine.setValue(group, "memorycue_next", 1);
        }
    }
};

PioneerDDJ400.memoryCueSet = function(_channel, _control, value, _status, group) {
    if (value) {
        engine.setValue(group, "memorycue_set", 1);
    }
};

PioneerDDJ400.memoryCueDelete = function(_channel, _control, value, _status, group) {
    if (value) {
        engine.setValue(group, "memorycue_delete", 1);
    }
};

//
// BEAT SYNC
//
// Note that the controller sends different signals for a short press and a long
// press of the same button.
//

PioneerDDJ400.syncPressed = function(channel, control, value, status, group) {
    if (value === 0) return; // ignore release
    
    // Toggle sync_enabled just like Rekordbox!
    const currentState = engine.getValue(group, "sync_enabled");
    engine.setValue(group, "sync_enabled", !currentState);
};

PioneerDDJ400.syncLongPressed = function(channel, control, value, status, group) {
    if (value) {
        engine.setValue(group, "sync_enabled", 1);
    }
};

PioneerDDJ400.cycleTempoRange = function(_channel, _control, value, _status, group) {
    if (value === 0) { return; } // ignore release

    const currRange = engine.getValue(group, "rateRange");
    let idx = 0;

    for (let i = 0; i < this.tempoRanges.length; i++) {
        if (currRange === this.tempoRanges[i]) {
            idx = (i + 1) % this.tempoRanges.length;
            break;
        }
    }
    engine.setValue(group, "rateRange", this.tempoRanges[idx]);
};

//
// Jog wheels
//

PioneerDDJ400.jogTurn = function(channel, _control, value, _status, group) {
    const deckNum = channel + 1;
    // wheel center at 64; <64 rew >64 fwd
    let newVal = value - 64;

    // loop_in / out adjust
    const loopEnabled = engine.getValue(group, "loop_enabled");
    if (loopEnabled > 0) {
        if (PioneerDDJ400.loopAdjustIn[channel]) {
            newVal = newVal * PioneerDDJ400.loopAdjustMultiply + engine.getValue(group, "loop_start_position");
            engine.setValue(group, "loop_start_position", newVal);
            return;
        }
        if (PioneerDDJ400.loopAdjustOut[channel]) {
            newVal = newVal * PioneerDDJ400.loopAdjustMultiply + engine.getValue(group, "loop_end_position");
            engine.setValue(group, "loop_end_position", newVal);
            return;
        }
    }

    if (engine.isScratching(deckNum)) {
        engine.scratchTick(deckNum, newVal);
    } else { // fallback
        engine.setValue(group, "jog", newVal * this.bendScale);
    }
};


PioneerDDJ400.jogSearch = function(_channel, _control, value, _status, group) {
    const newVal = (value - 64) * PioneerDDJ400.fastSeekScale;
    engine.setValue(group, "jog", newVal);
};

// Connection callback for [BiteDJ],vinyl_mode. Maps the CO (1 = Vinyl,
// 0 = CDJ) onto the boolean jogTouch() checks before enabling scratching.
PioneerDDJ400.setVinylMode = function(value) {
    PioneerDDJ400.vinylMode = value !== 0;
};

PioneerDDJ400.jogTouch = function(channel, _control, value) {
    const deckNum = channel + 1;

    // skip while adjusting the loop points
    if (PioneerDDJ400.loopAdjustIn[channel] || PioneerDDJ400.loopAdjustOut[channel]) {
        return;
    }

    if (value !== 0 && this.vinylMode) {
        engine.scratchEnable(deckNum, 720, 33+1/3, this.alpha, this.beta);
    } else {
        engine.scratchDisable(deckNum);
    }
};

//
// Shift button
//

PioneerDDJ400.shiftPressed = function(channel, _control, value, _status, _group) {
    PioneerDDJ400.shiftButtonDown[channel] = value === 0x7F;
};


//
// Tempo sliders
//
// The tempo option in Mixxx's deck preferences determine whether down/up
// increases/decreases the rate. Therefore it must be inverted here so that the
// UI and the control sliders always move in the same direction.
//

PioneerDDJ400.tempoSliderMSB = function(channel, control, value, status, group) {
    PioneerDDJ400.highResMSB[group].tempoSlider = value;
};

PioneerDDJ400.tempoSliderLSB = function(channel, control, value, status, group) {
    const fullValue = (PioneerDDJ400.highResMSB[group].tempoSlider << 7) + value;

    engine.setValue(
        group,
        "rate",
        1 - (fullValue / 0x2000)
    );
};

//
// Beat Jump mode
//
// Note that when we increase/decrease the sizes on the pad buttons, we use the
// value of the first pad (0x21) as an upper/lower limit beyond which we don't
// allow further increasing/decreasing of all the values.
//

PioneerDDJ400.beatjumpPadPressed = function(_channel, control, value, _status, group) {
    if (value === 0) {
        return;
    }
    engine.setValue(group, "beatjump_size", Math.abs(PioneerDDJ400.beatjumpSizeForPad[control]));
    engine.setValue(group, "beatjump", PioneerDDJ400.beatjumpSizeForPad[control]);
};

PioneerDDJ400.increaseBeatjumpSizes = function(_channel, control, value, _status, group) {
    if (value === 0 || PioneerDDJ400.beatjumpSizeForPad[0x21] * 16 > 16) {
        return;
    }
    Object.keys(PioneerDDJ400.beatjumpSizeForPad).forEach(function(pad) {
        PioneerDDJ400.beatjumpSizeForPad[pad] = PioneerDDJ400.beatjumpSizeForPad[pad] * 16;
    });
    engine.setValue(group, "beatjump_size", PioneerDDJ400.beatjumpSizeForPad[0x21]);
};

PioneerDDJ400.decreaseBeatjumpSizes = function(_channel, control, value, _status, group) {
    if (value === 0 || PioneerDDJ400.beatjumpSizeForPad[0x21] / 16 < 1/16) {
        return;
    }
    Object.keys(PioneerDDJ400.beatjumpSizeForPad).forEach(function(pad) {
        PioneerDDJ400.beatjumpSizeForPad[pad] = PioneerDDJ400.beatjumpSizeForPad[pad] / 16;
    });
    engine.setValue(group, "beatjump_size", PioneerDDJ400.beatjumpSizeForPad[0x21]);
};

//
// Sampler mode
//

PioneerDDJ400.samplerPlayOutputCallbackFunction = function(value, group, _control) {
    if (value === 1) {
        const curPad = group.match(script.samplerRegEx)[1];
        PioneerDDJ400.startSamplerBlink(
            0x97 + (curPad > 8 ? 2 : 0),
            0x30 + ((curPad > 8 ? curPad - 8 : curPad) - 1),
            group);
    }
};

PioneerDDJ400.samplerPadPressed = function(_channel, _control, value, _status, group) {
    if (engine.getValue(group, "track_loaded")) {
        engine.setValue(group, "cue_gotoandplay", value);
    } else {
        engine.setValue(group, "LoadSelectedTrack", value);
    }
};

PioneerDDJ400.samplerPadShiftPressed = function(_channel, _control, value, _status, group) {
    if (engine.getValue(group, "play")) {
        engine.setValue(group, "cue_gotoandstop", value);
    } else if (engine.getValue(group, "track_loaded")) {
        engine.setValue(group, "eject", value);
    }
};

PioneerDDJ400.startSamplerBlink = function(channel, control, group) {
    let val = 0x7f;

    PioneerDDJ400.stopSamplerBlink(channel, control);
    PioneerDDJ400.timers[channel][control] = engine.beginTimer(250, () => {
        val = 0x7f - val;

        // blink the appropriate pad
        midi.sendShortMsg(channel, control, val);
        // also blink the pad while SHIFT is pressed
        midi.sendShortMsg((channel+1), control, val);

        const isPlaying = engine.getValue(group, "play") === 1;

        if (!isPlaying) {
            // kill timer
            PioneerDDJ400.stopSamplerBlink(channel, control);
            // set the pad LED to ON
            midi.sendShortMsg(channel, control, 0x7f);
            // set the pad LED to ON while SHIFT is pressed
            midi.sendShortMsg((channel+1), control, 0x7f);
        }
    });
};

PioneerDDJ400.stopSamplerBlink = function(channel, control) {
    PioneerDDJ400.timers[channel] = PioneerDDJ400.timers[channel] || {};

    if (PioneerDDJ400.timers[channel][control] !== undefined) {
        engine.stopTimer(PioneerDDJ400.timers[channel][control]);
        PioneerDDJ400.timers[channel][control] = undefined;
    }
};

//
// Additional features
//

PioneerDDJ400.toggleQuantize = function(_channel, _control, value, _status, group) {
    if (value) {
        script.toggleControl(group, "quantize");
    }
};

PioneerDDJ400.quickJumpForward = function(_channel, _control, value, _status, group) {
    if (value) {
        engine.setValue(group, "beatjump", PioneerDDJ400.quickJumpSize);
    }
};

PioneerDDJ400.quickJumpBack = function(_channel, _control, value, _status, group) {
    if (value) {
        engine.setValue(group, "beatjump", -PioneerDDJ400.quickJumpSize);
    }
};

//
// Shutdown
//

PioneerDDJ400.shutdown = function() {
    // reset vumeter
    PioneerDDJ400.toggleLight(PioneerDDJ400.lights.deck1.vuMeter, false);
    PioneerDDJ400.toggleLight(PioneerDDJ400.lights.deck2.vuMeter, false);

    // housekeeping
    // turn off all Sampler LEDs
    for (let i = 0; i <= 7; ++i) {
        midi.sendShortMsg(0x97, 0x30 + i, 0x00);    // Deck 1 pads
        midi.sendShortMsg(0x98, 0x30 + i, 0x00);    // Deck 1 pads with SHIFT
        midi.sendShortMsg(0x99, 0x30 + i, 0x00);    // Deck 2 pads
        midi.sendShortMsg(0x9A, 0x30 + i, 0x00);    // Deck 2 pads with SHIFT
    }
    // turn off all Hotcue LEDs
    for (let i = 0; i <= 7; ++i) {
        midi.sendShortMsg(0x97, 0x00 + i, 0x00);    // Deck 1 pads
        midi.sendShortMsg(0x98, 0x00 + i, 0x00);    // Deck 1 pads with SHIFT
        midi.sendShortMsg(0x99, 0x00 + i, 0x00);    // Deck 2 pads
        midi.sendShortMsg(0x9A, 0x00 + i, 0x00);    // Deck 2 pads with SHIFT
    }

    // turn off loop in and out lights
    PioneerDDJ400.setLoopButtonLights(0x90, 0x00);
    PioneerDDJ400.setLoopButtonLights(0x91, 0x00);

    // turn off reloop lights
    PioneerDDJ400.setReloopLight(0x90, 0x00);
    PioneerDDJ400.setReloopLight(0x91, 0x00);

    // stop any flashing lights
    PioneerDDJ400.toggleLight(PioneerDDJ400.lights.beatFx, false);
    PioneerDDJ400.toggleLight(PioneerDDJ400.lights.shiftBeatFx, false);
};



// 1-indexed loaded_effect values (assuming standard alphabetical list)
// 9: Echo, 11: Flanger, 21: Reverb, 12: Glitch, 19: Phaser, 20: PitchShift
PioneerDDJ400.padFxPresets = [
    { effect: 15, beats: 0.25 },  // Pad 1: Echo 1/2 beat (approx)
    { effect: 15, beats: 0.5 },   // Pad 2: Echo 1 beat (approx)
    { effect: 15, beats: 0.75 },  // Pad 3: Echo 2 beats (approx)
    { effect: 15, beats: 1.0 },   // Pad 4: Echo 4 beats (approx)
    { effect: 13, beats: 0.25 },  // Pad 5: Flanger
    { effect: 13, beats: 0.5 },   // Pad 6: Flanger
    { effect: 13, beats: 1.0 },   // Pad 7: Flanger
    { effect: 3, beats: 0 }       // Pad 8: Reverb
];
PioneerDDJ400.padFxActiveCount = 0;
PioneerDDJ400.padFxSavedState = {};

PioneerDDJ400.padFxPressed = function(_channel, control, value, status, group) {
    let padIndex = -1;
    if (control >= 0x60 && control <= 0x67) {
        padIndex = control - 0x60;
    } else if (control >= 0x10 && control <= 0x17) {
        padIndex = control - 0x10;
    }
    if (padIndex < 0 || padIndex > 7) return;

    // VINYL BRAKE on Pad 8 (padIndex 7)
    if (padIndex === 7) {
        var deck = (_channel === 7 || _channel === 8) ? 1 : 2;
        if (value > 0) {
            engine.setValue("[Channel" + deck + "]", "brake", 1);
        } else {
            engine.setValue("[Channel" + deck + "]", "brake", 0);
        }
        return;
    }

    const fxGroup = "[EffectRack1_EffectUnit1_Effect1]";
    const unitGroup = "[EffectRack1_EffectUnit1]";

    if (value > 0) {
        if (PioneerDDJ400.padFxActiveCount === 0) {
            PioneerDDJ400.padFxSavedState = {
                enabled: engine.getValue(fxGroup, "enabled"),
                mix: engine.getValue(unitGroup, "mix"),
                meta: engine.getValue(fxGroup, "meta")
            };
        }
        PioneerDDJ400.padFxActiveCount++;

        const preset = PioneerDDJ400.padFxPresets[padIndex];
        engine.setValue(fxGroup, "loaded_effect", preset.effect);

        engine.beginTimer(20, function() {
            const paramIndex = PioneerDDJ400.findBeatsParameter(fxGroup);
            if (paramIndex !== -1 && preset.beats > 0) {
                engine.setValue(fxGroup, "parameter" + paramIndex + "_value", preset.beats);
            }
            engine.setValue(unitGroup, "mix", 1.0);
            engine.setValue(fxGroup, "meta", 1.0);
            engine.setValue(fxGroup, "enabled", 1);
        }, true);
        
    } else {
        if (PioneerDDJ400.padFxActiveCount > 0) {
            PioneerDDJ400.padFxActiveCount--;
        }
        if (PioneerDDJ400.padFxActiveCount === 0 && Object.keys(PioneerDDJ400.padFxSavedState).length > 0) {
            engine.setValue(unitGroup, "mix", PioneerDDJ400.padFxSavedState.mix);
            engine.setValue(fxGroup, "meta", PioneerDDJ400.padFxSavedState.meta);
            engine.setValue(fxGroup, "enabled", PioneerDDJ400.padFxSavedState.enabled);
            PioneerDDJ400.padFxSavedState = {};
        }
    }
};

PioneerDDJ400.crossfaderMoved = function(channel, control, value, status, group) {
    if (engine.getValue("[BiteDJ]", "crossfader_enabled") === 0) return;
    
    // Standard 7-bit MIDI mapping (0 to 127) -> (-1.0 to 1.0)
    engine.setValue("[Master]", "crossfader", (value / 63.5) - 1.0);
};


PioneerDDJ400.filterKnob = function(channel, control, value, status, group) {
    var deck = group === "[QuickEffectRack1_[Channel1]]" ? 1 : 2;
    var shift = PioneerDDJ400.shiftButtonDown[0] || PioneerDDJ400.shiftButtonDown[1];

    PioneerDDJ400.filterState = PioneerDDJ400.filterState || {};
    PioneerDDJ400.filterState[deck] = PioneerDDJ400.filterState[deck] || { msb: 64, lsb: 0 };
    
    if (control === 0x17 || control === 0x18) {
        PioneerDDJ400.filterState[deck].msb = value;
    } else if (control === 0x37 || control === 0x38) {
        PioneerDDJ400.filterState[deck].lsb = value;
    }
    
    var absVal = (PioneerDDJ400.filterState[deck].msb << 7) | PioneerDDJ400.filterState[deck].lsb;
    var normVal = absVal / 16383.0;

    if (shift) {
        // Shift + Filter (Deck 1 or Deck 2) controls the main Beat FX SUPER knob!
        // This allows simultaneous two-handed manipulation of both Super (left hand)
        // and Level/Depth Mix (right hand).
        engine.setValue("[EffectRack1_EffectUnit1]", "super1", normVal);
    } else {
        // Normal Filter knob controls the channel QuickEffect (super1)
        engine.setValue(group, "super1", normVal);
    }
};
