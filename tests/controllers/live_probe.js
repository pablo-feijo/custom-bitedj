// Test-only telemetry and fixture controls. Load after the shipped DDJ-400 scripts
// in isolated settings with synthetic music; never install in a user's preset.
var BiteDJProbe = {};
BiteDJProbe.snapshot = function() {
    const state = {time: Date.now(), grid: engine.getValue('[FxPanel]', 'grid')};
    for (let deck = 1; deck <= 2; ++deck) {
        const group = '[Channel' + deck + ']';
        state['d' + deck] = {};
        for (const key of ['playposition', 'beat_next', 'beat_distance', 'bpm',
            'rate', 'play', 'sync_enabled', 'scratch2', 'scratch2_enable', 'file_bpm']) {
            state['d' + deck][key] = engine.getValue(group, key);
        }
    }
    print('BITEDJ_PROBE ' + JSON.stringify(state));
};
BiteDJProbe.init = function() {
    BiteDJProbe.timer = engine.beginTimer(100, BiteDJProbe.snapshot);
};
BiteDJProbe.shutdown = function() { engine.stopTimer(BiteDJProbe.timer); };
BiteDJProbe.command = function(channel, control, value) {
    if (value <= 3) { engine.setValue('[FxPanel]', 'current', value); }
    if (value === 10) {
        for (let deck = 1; deck <= 2; ++deck) {
            const g = '[Channel' + deck + ']';
            engine.setValue(g, 'play', 0);
            engine.setValue(g, 'sync_enabled', 0);
            engine.setValue(g, 'quantize', 0);
            engine.setValue(g, 'bpm', engine.getValue(g, 'file_bpm'));
            engine.setValue(g, 'playposition', 0.25);
        }
    }
    if (value >= 20 && value <= 23) {
        engine.setValue('[BiteDJ]', 'vinyl_mode', value === 20 ? 0 : 1);
        engine.setValue('[BiteDJ]', 'vinyl_brake', value === 22 ? 1.8 : value === 23 ? 3.6 : 0);
    }
    if (value === 30 || value === 31) {
        for (let deck = 1; deck <= 2; ++deck) {
            engine.setValue('[Channel' + deck + ']', 'play', value === 30 ? 1 : 0);
        }
    }
    BiteDJProbe.snapshot();
};
