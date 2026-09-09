const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const root = path.resolve(__dirname, '../..');
let clock = 0, id = 0;
const values = new Map(), timers = new Map(), calls = [], leds = [];
const listeners = new Map();
const key = (g, k) => g + ':' + k;
const context = {console, Date: {now: () => clock}, midi: {sendShortMsg: (...v) => leds.push(v)}, engine: {
    getValue: (g,k) => values.get(key(g,k)) ?? 0,
    setValue: (g,k,v) => {values.set(key(g,k),v); calls.push([g,k,v]); (listeners.get(key(g,k)) || []).forEach(fn=>fn(v));},
    makeConnection: (g,k,fn) => {
        const list = listeners.get(key(g,k)) || []; list.push(fn); listeners.set(key(g,k),list);
        return {disconnect:()=>list.splice(list.indexOf(fn),1)};
    },
    beginTimer: (ms,fn,once) => {const timerId=++id; timers.set(timerId,{ms,fn:()=>{if(once)timers.delete(timerId); fn();},once}); return timerId;},
    stopTimer: id => timers.delete(id)
}};
vm.createContext(context);
vm.runInContext(fs.readFileSync(path.join(root,'res/controllers/piflex-padfx.js'),'utf8'), context);
const fx = context.PiFlexPadFx;
for(let n=1;n<=4;n++) {
    const deck=`[Channel${n}]`;
    values.set(key(deck,'play'),1); values.set(key(deck,'bpm'),140);
    fx.lanes.forEach(lane=>values.set(key(fx.group(deck,lane),'available'),1));
}
fx.init();
const deck='[Channel1]', echo=fx.group(deck,'echo');
const get=(g,k)=>values.get(key(g,k));
const press=(pad,down=true,shift=false,d=deck)=>fx.press(0x10+pad,down?127:0,shift?0x98:0x97,d);
press(4); assert.equal(get(echo,'param_delay_time'),.25);
press(5); assert.equal(get(echo,'param_delay_time'),.5);
press(5,false); assert.equal(get(echo,'param_delay_time'),.25); // earlier held wins again
press(4,false,true); // SHIFT changed before note-off
assert.equal(get(echo,'active'),1); assert.equal(get(echo,'param_send_amount'),0);
const oldTail=timers.get(fx.decks[deck].jobs['echo:tail'].id).fn;
press(5); oldTail(); assert.equal(get(echo,'active'),1); // stale timer cannot cut retrigger
press(1); assert.equal(get(fx.group(deck,'sweep'),'active'),1); // different FX combine
press(5,false); assert.equal(get(fx.group(deck,'sweep'),'active'),1);
press(1,false);
press(7); const capture=timers.get(fx.decks[deck].jobs['echo:capture'].id).fn;
capture(); assert.equal(get(echo,'param_dry_amount'),0);
assert.equal(get(deck,'volume'),undefined); // Release FX never fights fader
press(7,false); assert.equal(get(echo,'param_dry_amount'),1);
assert.equal(get(echo,'param_send_amount'),0); assert.equal(get(echo,'active'),1);
for(let n=1;n<=4;n++) {
    const d=`[Channel${n}]`;
    for(let p=0;p<16;p++) {
        const physical=p%8, status=0x95+n*2+(p>=8?1:0);
        fx.press(0x10+physical,127,status,d);
        assert.equal(Object.keys(fx.decks[d].held).length>0,true,`deck ${n} pad ${p}`);
        // duplicate NoteOn is idempotent
        const count=Object.keys(fx.decks[d].held).length;
        fx.press(0x10+physical,127,status,d);
        assert.equal(Object.keys(fx.decks[d].held).length,count);
        fx.press(0x10+physical,0,status,d);
        assert.equal(Object.keys(fx.decks[d].held).length,0);
        assert.equal(get(d,'scratch2_enable')||0,0);
    }
}
assert.ok(!calls.some(([g,k])=>g.includes('QuickEffect')||k==='loaded_chain_preset'||k==='volume'||k==='play'));
// Release cancels earlier holds and their tails; late note-offs cannot undo it.
press(1); press(6); press(7); press(1,false); press(6,false);
assert.equal(get(echo,'active'),1); assert.equal(Object.keys(fx.decks[deck].held).length,1);
press(7,false);
fx.shutdown(); assert.equal(timers.size,0);
fx.init(); values.set(key(fx.group(deck,'crush'),'available'),0);
press(1,true,true); assert.equal(Object.keys(fx.decks[deck].held).length,0);
fx.shutdown();
// Versioned editor controls are read only on a new press, never on note-off.
values.set(key('[PadFX]','version'),1);
for(let n=1;n<=4;n++) for(let p=0;p<16;p++) {
    for(const [field,value] of Object.entries({effect:p,beat:0,strength:4,hold:0})) {
        values.set(key('[PadFX]',`d${n}_s${p}_${field}`),value);
    }
}
values.set(key(fx.group(deck,'crush'),'available'),1);
fx.init();
const setting=(slot,field,value,n=1)=>values.set(key('[PadFX]',`d${n}_s${slot}_${field}`),value);
setting(1,'effect',4); setting(1,'beat',6); setting(1,'strength',2);
press(1); assert.equal(get(echo,'param_delay_time'),2);
assert.equal(get(echo,'param_send_amount'),.11);
setting(1,'effect',9); press(1,false,true);
assert.equal(get(echo,'param_send_amount'),0); // releases old Echo, not new Crush
press(1); assert.equal(get(fx.group(deck,'crush'),'param_bit_depth'),13);
assert.equal(get(fx.group(deck,'crush'),'param_downsample'),.725);
press(1,false);
press(1,true,false,'[Channel2]');
assert.equal(get(fx.group('[Channel2]','sweep'),'active'),1); // per-deck isolation
press(1,false,false,'[Channel2]');
setting(7,'hold',1); press(7); press(7); press(7,false);
assert.ok(fx.decks[deck].held[7]); // duplicate down ignored, note-off keeps latch
const captured=timers.get(fx.decks[deck].jobs['echo:capture'].id).fn;
captured(); assert.equal(get(echo,'param_dry_amount'),0);
setting(7,'effect',1); press(7,true,true); // same physical pad unlatches despite Shift/edit
assert.equal(get(echo,'param_dry_amount'),1); assert.equal(fx.decks[deck].held[7],undefined);
press(7,false);
setting(7,'effect',7); press(7); press(7,false);
context.engine.setValue(deck,'track_loaded',0);
assert.equal(Object.keys(fx.decks[deck].held).length,0); // no latch on next track
assert.equal(get(echo,'active'),0);
press(7); press(7,false);
context.engine.setValue('[PadFX]','clear_all',1);
assert.equal(Object.keys(fx.decks[deck].held).length,0);
assert.equal(get(echo,'active'),0); assert.equal(timers.size,0);
captured(); assert.equal(get(echo,'active'),0); // stale callbacks after panic do nothing
setting(7,'strength',0); press(7);
assert.equal(Object.keys(fx.decks[deck].held).length,0); press(7,false);
setting(7,'strength',4); setting(7,'effect',16); press(7);
assert.equal(Object.keys(fx.decks[deck].held).length,0); press(7,false);
setting(3,'hold',1); press(3); press(3,false);
assert.equal(get(deck,'scratch2_enable'),0); // never latch transport
setting(1,'effect',0); setting(1,'strength',4);
press(0); press(1); assert.equal(Object.keys(fx.decks[deck].held).length,1);
press(1,false); assert.equal(get(deck,'beatlooproll_0.5_activate'),1);
press(0,false); assert.equal(get(deck,'beatlooproll_0.5_activate'),0);
// Corrupt values fall back independently. None can index an undefined preset.
for(const bad of [NaN,Infinity,-1,123,.25]) {
    setting(4,'effect',bad); setting(4,'beat',bad); setting(4,'strength',bad); setting(4,'hold',bad);
    press(4); assert.equal(get(echo,'param_delay_time'),.25);
    assert.equal(get(echo,'param_send_amount'),.22); press(4,false);
}
fx.shutdown(); assert.equal(timers.size,0);
assert.ok([...listeners.values()].every(list=>list.length===0));
// Future control contract is not interpreted as v1 (and is never overwritten).
values.set(key('[PadFX]','version'),2); fx.init(); press(4);
assert.equal(get(echo,'param_delay_time'),.25); press(4,false); fx.shutdown();
assert.ok(!calls.some(([g,k])=>g==='[PadFX]' && k!=='clear_all'));
// Exercise our DDJ-400 adapter, including Shift shadow notes and LED routing.
const mapping = fs.readFileSync(path.join(root, 'res/controllers/Pioneer-DDJ-400-script.js'), 'utf8');
context.PioneerDDJ400 = {};
const adapter = mapping.slice(mapping.indexOf('PioneerDDJ400.padFxPressed ='), mapping.indexOf('PioneerDDJ400.crossfaderMoved ='));
vm.runInContext(adapter, context);
const routed = [];
const originalPress = fx.press;
fx.press = (...args) => routed.push(args);
for (const [channel, note, expectedDeck] of [[7,0x10,'[Channel1]'],[8,0x60,'[Channel1]'],[9,0x17,'[Channel2]'],[10,0x67,'[Channel2]']]) {
    context.PioneerDDJ400.padFxPressed(channel, note, 127, 0x90+channel, expectedDeck);
    context.PioneerDDJ400.padFxPressed(channel, note, 0, 0x80+channel, expectedDeck);
    assert.deepEqual(routed.at(-2), [note >= 0x60 ? note-0x50 : note,127,0x90+channel,expectedDeck,note]);
    assert.equal(routed.at(-1)[1],0);
}
fx.press = originalPress;
const xml = fs.readFileSync(path.join(root,'res/controllers/Pioneer-DDJ-400.midi.xml'),'utf8');
assert.ok(xml.includes('filename="piflex-padfx.js"'));
assert.ok(fs.readFileSync(path.join(root,'src/effects/chains/padeffectchain.cpp'),'utf8').includes('SignalProcessingStage::Postfader'));
console.log('PASS: ordered holds, tails, retrigger, shifted note-off, snapshots, strength, timing, latch, stop, corruption, shutdown and DDJ-400 adapter');

fx.init(2);
assert.deepEqual(Object.keys(fx.decks), ['[Channel1]', '[Channel2]']);
fx.press(0x10,127,0x98,deck,0x60);
assert.deepEqual(leds.at(-1), [0x98,0x60,0x7f]);
fx.press(0x10,0,0x88,deck,0x60);
assert.deepEqual(leds.at(-1), [0x98,0x60,0]);
fx.shutdown();
