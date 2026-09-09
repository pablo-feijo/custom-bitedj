const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const values = new Map();
const writes = [];
const group = '[EffectRack1_EffectUnit1_Effect1]';
const context = {engine: {
    getValue(g, k) { if (g === '[EffectRack1_EffectUnit1]' && k === 'focused_effect') return 1; assert.equal(g, group); return values.get(k) ?? 0; },
    setValue(g, k, v) { assert.equal(g, group); values.set(k, v); writes.push(v); }
}};
vm.createContext(context);
vm.runInContext(fs.readFileSync('res/controllers/Pioneer-DDJ-400-script.js', 'utf8'), context);
const controller = context.PioneerDDJ400;
for (const max of [2, 4]) {
    values.set('parameter2_loaded', 1);
    values.set('parameter2_units', 1);
    values.set('parameter2_beat_period_min', 0.125);
    values.set('parameter2_beat_period_max', max);
    values.set('parameter2_beat_period', 0.125);
    writes.length = 0;
    for (let i = 0; i < 8; ++i) controller.beatFxRightPressed(0, 0, 127);
    assert.deepEqual(writes, max === 2 ? [0.25, 0.5, 1, 2] : [0.25, 0.5, 1, 2, 4]);
    controller.beatFxLeftPressed(0, 0, 0);
    assert.equal(values.get('parameter2_beat_period'), max, 'release does nothing');
    // A touch change is read on the next controller press.
    values.set('parameter2_beat_period', 0.5);
    controller.beatFxLeftPressed(0, 0, 127);
    assert.equal(values.get('parameter2_beat_period'), 0.25);
}
values.set('parameter2_units', 0);
writes.length = 0;
controller.beatFxRightPressed(0, 0, 127);
assert.equal(writes.length, 0);
console.log('DDJ-400 Beat period range and touch/controller synchronization passed.');
