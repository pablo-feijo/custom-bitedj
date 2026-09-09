const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const root = path.resolve(__dirname, '../..');
const values = new Map(), listeners = new Map(), writes = [], timers = new Map();
let timerId = 0;
const key = (g,k) => `${g}:${k}`;
const engine = {
    getValue: (g,k) => values.get(key(g,k)) ?? 0,
    setValue(g,k,v) {
        const previous = this.getValue(g,k);
        values.set(key(g,k),v); writes.push([g,k,v]);
        if (previous !== v) for (const fn of [...(listeners.get(key(g,k)) || [])]) fn(v);
    },
    makeConnection(g,k,fn) {
        const list = listeners.get(key(g,k)) || []; list.push(fn); listeners.set(key(g,k),list);
        return {disconnect: () => list.splice(list.indexOf(fn),1)};
    },
    beginTimer: (ms,fn) => { timers.set(++timerId,fn); return timerId; },
    stopTimer: id => timers.delete(id)
};
const context = vm.createContext({engine, console, Date});
for (const file of ['piflex-padfx.js','piflex-touch-pads.js'])
    vm.runInContext(fs.readFileSync(path.join(root,'res/controllers',file),'utf8'),context);
const fx = context.PiFlexPadFx;
engine.setValue('[PadFX]','version',1);
for (let n=1;n<=2;n++) {
    const group = `[Channel${n}]`;
    engine.setValue(group,'play',1); engine.setValue(group,'bpm',120);
    engine.setValue('[PadFX]',`d${n}_jump_bank`,1);
    for (const lane of fx.lanes) engine.setValue(fx.group(group,lane),'available',1);
    for (let slot=0;slot<16;slot++) for (const [field,value] of Object.entries({effect:slot,beat:0,strength:4,hold:0}))
        engine.setValue('[PadFX]',`d${n}_s${slot}_${field}`,value);
}
context.PiFlexTouchPads.init();
engine.setValue('[PadFX]','runtime_available',1);
const set = (n,k,v) => engine.setValue('[PadFX]',`d${n}_${k}`,v);
const touch = (n,p,v) => set(n,`touch_p${p}`,v);
for (let n=1;n<=2;n++) {
    const group = `[Channel${n}]`;
    set(n,'mode',2);
    for (let bank=0;bank<3;bank++) {
        set(n,'jump_bank',bank);
        for (let pad=0;pad<8;pad++) {
            touch(n,pad,1); touch(n,pad,0);
            const size = [1/16,1,16][bank] * 2**Math.floor(pad/2);
            assert.equal(engine.getValue(group,'beatjump'),size*(pad%2?1:-1));
        }
    }
    set(n,'mode',3);
    for (let pad=0;pad<8;pad++) {
        const control = (pad<4?'beatlooproll_':'beatloop_') + [0.25,0.5,1,2,4,8,16,32][pad] + (pad<4?'_activate':'_toggle');
        touch(n,pad,1); assert.equal(engine.getValue(group,control),1);
        touch(n,pad,0); assert.equal(engine.getValue(group,control),0);
    }
    touch(n,0,1); set(n,'mode',1); // cancel held roll on mode switch
    assert.equal(engine.getValue(group,'beatlooproll_0.25_activate'),0);
    touch(n,0,0);
    for (let shifted=0;shifted<2;shifted++) {
        set(n,'shift',shifted);
        for (let p=0;p<8;p++) {
            touch(n,p,1);
            assert.ok(fx.decks[group].held[`touch${p}`],`deck ${n} shift ${shifted} pad ${p}`);
            touch(n,p,0);
            assert.equal(fx.decks[group].held[`touch${p}`],undefined);
        }
    }
    set(n,'shift',0);
}
// FX 2 remains the second assignment bank after the selecting Shift is released.
for (const n of [1,2]) {
    set(n,'mode',5); set(n,'shift',0);
    for (let p=0;p<8;p++) {
        touch(n,p,1);
        assert.equal(fx.decks[`[Channel${n}]`].held[`touch${p}`].pad.name,fx.pads[p+8].name);
        touch(n,p,0);
    }
}
// Same effect, same physical pad, two input owners. Releasing one preserves the other.
set(1,'mode',1); touch(1,1,1); set(1,'hardware_p1',1);
const sweep=fx.group('[Channel1]','sweep');
touch(1,1,0); assert.equal(engine.getValue(sweep,'active'),1);
set(1,'hardware_p1',0); assert.equal(engine.getValue(sweep,'active'),0);
// Remapping a held touch cannot redirect its release.
touch(1,4,1); set(1,'s4_effect',1); touch(1,4,0);
assert.equal(engine.getValue(fx.group('[Channel1]','echo'),'param_send_amount'),0);
// Shift change cancels touch on its original bank.
touch(1,1,1); set(1,'shift',1); assert.equal(engine.getValue(sweep,'active'),0); touch(1,1,0);
// Shift Beat Loop uses shifted FX; Shift Jump changes the shared bounded bank.
set(1,'mode',3); touch(1,1,1); assert.equal(engine.getValue(fx.group('[Channel1]','crush'),'active'),1); touch(1,1,0);
set(1,'mode',2);
for(let i=0;i<4;i++){touch(1,6,1);touch(1,6,0);}
assert.equal(engine.getValue('[PadFX]','d1_jump_bank'),0);
assert.equal(engine.getValue('[PadFX]','d2_jump_bank'),0);
for(let i=0;i<4;i++){touch(1,7,1);touch(1,7,0);}
assert.equal(engine.getValue('[PadFX]','d1_jump_bank'),2);
// Release Echo toggle still latches until the second touch or emergency clear.
set(1,'mode',1); set(1,'shift',0); set(1,'s7_hold',1);
touch(1,7,1); touch(1,7,0); assert.ok(fx.decks['[Channel1]'].held.touch7);
touch(1,7,1); touch(1,7,0); assert.equal(fx.decks['[Channel1]'].held.touch7,undefined);
// Mapping facade forwards normal/shift/releases into the already-running system.
const ledWrites = [];
const controller=vm.createContext({engine,console,Date,midi:{sendShortMsg(...args){ledWrites.push(args);}}});
vm.runInContext(fs.readFileSync(path.join(root,'res/controllers/piflex-padfx.js'),'utf8'),controller);
controller.PiFlexPadFx.init(2);
controller.PiFlexPadFx.press(0x11,127,0x98,'[Channel2]');
assert.ok(fx.decks['[Channel2]'].held.hardware1);
assert.equal(fx.decks['[Channel2]'].held.hardware1.pad.lane,'crush');
controller.PiFlexPadFx.press(0x11,0,0x97,'[Channel2]');
assert.equal(fx.decks['[Channel2]'].held.hardware1,undefined);
touch(1,1,1); controller.PiFlexPadFx.press(0x11,127,0x97,'[Channel1]');
controller.PiFlexPadFx.shutdown(); assert.equal(engine.getValue(sweep,'active'),1);
touch(1,1,0); assert.equal(engine.getValue(sweep,'active'),0);
assert.ok(ledWrites.some(([status,note,value]) => status === 0x98 && note === 0x11 && value === 127));
controller.PiFlexPadFx.init(2);
controller.PiFlexPadFx.press(0x17,127,0x97,'[Channel1]');
controller.PiFlexPadFx.press(0x17,0,0x97,'[Channel1]');
assert.equal(ledWrites.at(-1)[2],127); // latched echo keeps its LED after release
controller.PiFlexPadFx.press(0x17,127,0x97,'[Channel1]');
assert.equal(ledWrites.at(-1)[2],0);
controller.PiFlexPadFx.press(0x17,0,0x97,'[Channel1]');
controller.PiFlexPadFx.shutdown();
touch(1,1,0); assert.equal(engine.getValue(sweep,'active'),0);
// A hardware jump immediately adopts a bank selected from the touchscreen.
controller.script = {};
vm.runInContext(fs.readFileSync(path.join(root,'res/controllers/Pioneer-DDJ-400-script.js'),'utf8'),controller);
set(1,'jump_bank',0);
controller.PioneerDDJ400.beatjumpPadPressed(0,0x27,127,0x97,'[Channel1]');
assert.equal(engine.getValue('[Channel1]','beatjump'),0.5);
controller.PioneerDDJ400.increaseBeatjumpSizes(0,0,127,0,'[Channel1]');
assert.equal(engine.getValue('[PadFX]','d1_jump_bank'),1);
assert.equal(engine.getValue('[PadFX]','d2_jump_bank'),1);
context.PiFlexTouchPads.shutdown();
assert.equal(timers.size,0);
assert.ok(!writes.some(([g,k])=>g.includes('QuickEffect')||k==='loaded_chain_preset'||k==='volume'));
console.log('PASS: touch all modes/banks/decks, holds, cancellation, remapping, toggles, shared MIDI ownership and shutdown');
