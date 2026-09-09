// System-owned performance input: available without a MIDI device. Both touch
// and controller FX use one lane/timer owner, with distinct held-key identities.
var midi = {sendShortMsg: function(status, control, value) {
    var deck = (status & 0x0F) <= 8 ? 1 : 2;
    engine.setValue("[PadFX]", "d" + deck + "_hardware_led" + (control - 0x10), value ? 1 : 0);
}};
var PiFlexTouchPads = {
    connections: [],
    held: {},
    init: function() {
        PiFlexPadFx.init(2);
        var self = this;
        for (var deck = 1; deck <= 2; ++deck) {
            (function(n) {
                var group = "[Channel" + n + "]", prefix = "d" + n + "_";
                for (var pad = 0; pad < 8; ++pad) {
                    (function(p) {
                        self.connections.push(engine.makeConnection("[PadFX]", prefix + "touch_p" + p,
                            function(value) { self.touch(n, p, value); }));
                        self.connections.push(engine.makeConnection("[PadFX]", prefix + "hardware_p" + p,
                            function(value) { PiFlexPadFx.press(0x10 + p, value ? 127 : 0,
                                (n === 1 ? 0x97 : 0x99) + (value === 2 ? 1 : 0), group, undefined, "hardware"); }));
                    })(pad);
                }
                self.connections.push(engine.makeConnection("[PadFX]", prefix + "hardware_clear", function(value) {
                    if (value !== 1) return;
                    var state = PiFlexPadFx.decks[group];
                    Object.keys(state.held).forEach(function(key) {
                        if (key.indexOf("hardware") === 0) PiFlexPadFx.release(group, key, true);
                    });
                    for (var p = 0; p < 8; ++p) {
                        delete state.pressed["hardware" + p];
                        engine.setValue("[PadFX]", prefix + "hardware_p" + p, 0);
                    }
                }));
                ["mode", "shift"].forEach(function(key) {
                    self.connections.push(engine.makeConnection("[PadFX]", prefix + key,
                        function() { self.releaseDeck(n); }));
                });
                self.connections.push(engine.makeConnection(group, "track_loaded", function(value) {
                    if (!value) self.releaseDeck(n);
                }));
            })(deck);
        }
        this.connections.push(engine.makeConnection("[PadFX]", "clear_all", function(value) {
            if (value === 1) { self.releaseDeck(1); self.releaseDeck(2); }
        }));
    },
    releaseDeck: function(deck) {
        for (var p = 0; p < 8; ++p) this.touch(deck, p, 0);
    },
    touch: function(deck, pad, value) {
        var key = deck + ":" + pad, group = "[Channel" + deck + "]", prefix = "d" + deck + "_";
        if (!value) {
            var held = this.held[key];
            delete this.held[key];
            if (!held) return;
            if (held.fx) PiFlexPadFx.press(0x10 + pad, 0, 0x97, group, undefined, "touch");
            else if (held.control) engine.setValue(group, held.control, 0);
            return;
        }
        if (this.held[key]) return;
        var mode = engine.getValue("[PadFX]", prefix + "mode");
        var shift = engine.getValue("[PadFX]", prefix + "shift") !== 0;
        var held = {};
        this.held[key] = held;
        if (mode === 1 || mode === 5 || (mode === 3 && shift)) {
            held.fx = true;
            PiFlexPadFx.press(0x10 + pad, 127, mode === 5 || shift ? 0x98 : 0x97, group, undefined, "touch");
        } else if (mode === 2) {
            var bank = engine.getValue("[PadFX]", prefix + "jump_bank");
            if (shift) {
                if (pad < 6) return;
                bank = Math.max(0, Math.min(2, bank + (pad === 6 ? -1 : 1)));
                // Controller jump-size bank is shared across the two decks.
                engine.setValue("[PadFX]", "d1_jump_bank", bank);
                engine.setValue("[PadFX]", "d2_jump_bank", bank);
            } else {
                var size = [1 / 16, 1, 16][bank] * Math.pow(2, Math.floor(pad / 2));
                engine.setValue(group, "beatjump_size", size);
                engine.setValue(group, "beatjump", pad % 2 ? size : -size);
            }
        } else if (mode === 3) {
            held.control = (pad < 4 ? "beatlooproll_" : "beatloop_") +
                [0.25, 0.5, 1, 2, 4, 8, 16, 32][pad] + (pad < 4 ? "_activate" : "_toggle");
            engine.setValue(group, held.control, 1);
        }
    },
    shutdown: function() {
        this.releaseDeck(1);
        this.releaseDeck(2);
        this.connections.forEach(function(connection) { connection.disconnect(); });
        this.connections = [];
        PiFlexPadFx.shutdown();
    }
};
