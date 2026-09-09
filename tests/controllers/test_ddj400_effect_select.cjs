const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '../..');
const group = '[EffectRack1_EffectUnit1]';
let current, writes;
const context = {engine: {
    getValue(g, key) {
        if (g === '[Skin]' && key === 'cue_panel') return 0;
        assert.equal(g, group);
        assert.equal(key, 'chain_selector');
        assert.fail('relative selection must not read chain_selector as an index');
    },
    setValue(g, key, value) {
        if (g === '[PadFX]') {
            assert.match(key, /^d[12]_shift$/);
            assert.ok(value === 0 || value === 1);
            return;
        }
        assert.equal(g, group);
        assert.equal(key, 'chain_selector');
        current = value;
        writes.push(value);
    }
}};
vm.createContext(context);
vm.runInContext(fs.readFileSync(path.join(root, 'res/controllers/Pioneer-DDJ-400-script.js'), 'utf8'), context);
const xml = fs.readFileSync(path.join(root, 'res/controllers/Pioneer-DDJ-400.midi.xml'), 'utf8');
const controls = [...xml.matchAll(/<control>([\s\S]*?)<\/control>/g)].map(([_, body]) => {
    const tag = name => body.match(new RegExp(`<${name}>([^<]+)</${name}>`))?.[1];
    return {status: Number(tag('status')), note: Number(tag('midino')), key: tag('key'), group: tag('group')};
});
function send(status, note, value) {
    const bindings = controls.filter(c => c.status === status && c.note === note);
    assert.equal(bindings.length, 1, 'one MIDI binding per tested input');
    const binding = bindings[0];
    const callback = binding.key.split('.').reduce((obj, key) => obj[key], context);
    callback(status & 0x0f, note, value, status, binding.group);
}
function reset(start) {
    current = start;
    writes = [];
    send(0x90, 0x3f, 0);
    send(0x91, 0x3f, 0);
}
for (const shifts of [[], [0x90], [0x91], [0x90, 0x91]]) {
    reset(0);
    shifts.forEach(status => send(status, 0x3f, 127));
    send(0x94, 0x63, 127);
    assert.deepEqual(writes, [shifts.length ? -1 : 1]);
    shifts.forEach(status => send(status, 0x3f, 0));
    send(0x94, 0x63, 0);
    assert.equal(writes.length, 1, 'release after Shift changes must do nothing');
    send(0x94, 0x63, 127);
    assert.equal(writes[1], 1, 'unshifted selection returns to forward');
}
for (const shifted of [false, true]) {
    reset(0);
    if (shifted) send(0x90, 0x3f, 127);
    send(0x94, 0x64, 127);
    send(0x94, 0x64, 0);
    assert.deepEqual(writes, [-1], 'dedicated shifted note sends one reverse step');
}
reset(0);
send(0x90, 0x3f, 127);
send(0x91, 0x3f, 127);
send(0x90, 0x3f, 0);
send(0x94, 0x63, 127);
assert.deepEqual(writes, [-1], 'remaining held Shift still reverses selection');
// ControlEncoder input is a direction, never the current preset index.
console.log('DDJ-400 effect-selection MIDI direction checks passed.');
