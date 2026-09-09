const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const root = path.resolve(__dirname, '../..');
const values = new Map(), writes = [];
const key = (g,k) => `${g}:${k}`;
const engine = {
    getValue: (g,k) => values.get(key(g,k)) || 0,
    setValue: (g,k,v) => { values.set(key(g,k),v); writes.push([g,k,v]); },
};
const context = {engine, console, script:{}, midi:{}};
vm.createContext(context);
vm.runInContext(fs.readFileSync(path.join(root,'res/controllers/Pioneer-DDJ-400-script.js'),'utf8'),context);
const m=context.PioneerDDJ400;
const read=(d,k)=>engine.getValue('[PadFX]',`d${d}_${k}`);
const select=(d,n,v=127,status=0x8f+d)=>m.padModeSelected(d-1,n,v,status,`[Channel${d}]`);
m.resetPadDisplay();
for (const deck of [1,2]) {
    for (const [note,mode] of [[0x1e,1],[0x20,2],[0x6d,3],[0x1b,0]]) {
        select(deck,note);
        assert.equal(read(deck,'mode'),mode);
        assert.equal(engine.getValue('[Skin]',`cue_deck${deck}`),1);
        const count=writes.length;
        select(deck,note,0);
        select(deck,note,64,0x7f+deck); // Real note-off may carry release velocity.
        assert.equal(writes.length,count);
    }
}
select(1,0x1e);select(2,0x20);
assert.equal(read(1,'mode'),1);assert.equal(read(2,'mode'),2);
m.shiftPressed(0,0x3f,127);assert.equal(read(1,'shift'),1);assert.equal(read(2,'shift'),0);
m.shiftPressed(0,0x3f,0);assert.equal(read(1,'shift'),0);
// Another deck's sampler selection cannot close the visible drawer.
engine.setValue('[Skin]','cue_panel',1);engine.setValue('[Skin]','cue_close',0);
select(2,0x22);assert.equal(engine.getValue('[Skin]','cue_close'),0);
select(1,0x22);assert.equal(engine.getValue('[Skin]','cue_close'),1);
for (const note of [0x69,0x6b,0x6f]) {
    select(1,0x1e);select(1,note);assert.equal(read(1,'mode'),0);
}
const count=writes.length;select(1,0x7e);assert.equal(writes.length,count);
m.increaseBeatjumpSizes(0,0,127,0x90,'[Channel1]');
assert.equal(read(1,'jump_bank'),2);assert.equal(read(2,'jump_bank'),2);
m.decreaseBeatjumpSizes(0,0,127,0x90,'[Channel1]');
m.decreaseBeatjumpSizes(0,0,127,0x90,'[Channel1]');
assert.equal(read(1,'jump_bank'),0);assert.equal(read(2,'jump_bank'),0);
m.resetPadDisplay();assert.equal(read(1,'mode'),0);assert.equal(read(2,'mode'),0);
// Confirm real MIDI bindings, not just direct callback behavior.
const xml=fs.readFileSync(path.join(root,'res/controllers/Pioneer-DDJ-400.midi.xml'),'utf8');
const controls=[...xml.matchAll(/<control>([\s\S]*?)<\/control>/g)].map(m=>m[1]);
for(const deck of [1,2]) for(const note of [0x1b,0x1e,0x20,0x6d,0x69,0x6b,0x22,0x6f]) {
    const matches=controls.filter(c=>c.includes(`<status>0x${(0x8f+deck).toString(16).toUpperCase()}</status>`) && c.includes(`<midino>0x${note.toString(16).toUpperCase()}</midino>`));
    assert.equal(matches.length,1);
    assert.ok(matches[0].includes('<key>PioneerDDJ400.padModeSelected</key>'));
}
assert.ok(!writes.some(([g,k])=>g.startsWith('[Channel') && !['beatjump_size'].includes(k)));
console.log('PASS: mode activation, both decks, releases, Shift, other modes, jump banks and MIDI bindings');
