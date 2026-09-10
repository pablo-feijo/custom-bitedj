const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '../..');
const xml = fs.readFileSync(path.join(root, 'res/controllers/Pioneer-DDJ-400.midi.xml'), 'utf8');
const controls = [...xml.matchAll(/<control>([\s\S]*?)<\/control>/g)].map(([, body]) => {
    const tag = name => body.match(new RegExp(`<${name}>([^<]+)</${name}>`))?.[1];
    return {status: Number(tag('status')), note: Number(tag('midino')), group: tag('group'),
        key: tag('key'), options: [...(body.match(/<options>([\s\S]*?)<\/options>/)?.[1] || '').matchAll(/<([\w-]+)\s*\/>/g)].map(([,name])=>name.toLowerCase()), scripted: /<script-binding\s*\/>/i.test(body)};
});
const exercised = new Set();
function createHarness() {
    let now = 10000, nextTimer = 0;
    const values = new Map(), writes = [], leds = [], calls = [], timers = new Map(), listeners = [];
    const key = (g,k) => `${g}:${k}`;
    const get = (g,k) => values.get(key(g,k)) ?? 0;
    const seed = (g,k,v) => values.set(key(g,k),v);
    const set = (g,k,v) => {
        seed(g,k,v); writes.push([g,k,v]);
        if (k === "sync_leader" && v > 0) seed(g,"sync_enabled",1);
        if (g === '[Skin]' && v === 1) {
            if (k === 'cue_close') seed(g,'cue_panel',0);
            if (/^cue_deck[12]$/.test(k)) seed(g,'cue_panel',Number(k.at(-1)));
        }
        listeners.filter(c => c.active && c.g === g && c.k === k).forEach(c => c.fn(v,g,k));
    };
    const connect = (g,k,fn) => {
        const c = {g,k,fn,active:true}; listeners.push(c);
        return {trigger: () => fn(get(g,k),g,k), disconnect: () => {c.active=false;}};
    };
    const engine = {getValue:get, setValue:set, setParameter:set,
        makeConnection:connect, makeUnbufferedConnection:connect,
        beginTimer: (ms,fn,once) => { const id=++nextTimer; timers.set(id,{ms,fn,once}); return id; },
        stopTimer: id => timers.delete(id),
        isScratching: d => !!get(`[Channel${d}]`,'test_scratching')};
    for (const name of ['softTakeover','softTakeoverIgnoreNextValue','scratchEnable','scratchDisable','scratchTick']) {
        engine[name] = (...args) => calls.push([name,...args]);
    }
    const context = {console, print:()=>{}, Date:{now:()=>now}, engine,
        midi:{sendShortMsg:(...args)=>leds.push(args),sendSysexMsg:(...args)=>calls.push(['sysex',...args])},
        script:{channelRegEx:/\[Channel(\d+)\]/,samplerRegEx:/\[Sampler(\d+)\]/,
            toggleControl:(g,k)=>set(g,k,get(g,k)?0:1),
            triggerControl:(g,k,ms)=>calls.push(['triggerControl',g,k,ms])}};
    vm.createContext(context);
    for (const file of ['piflex-padfx.js','Pioneer-DDJ-400-script.js']) {
        vm.runInContext(fs.readFileSync(path.join(root,'res/controllers',file),'utf8'),context,{filename:file});
    }
    const m=context.PioneerDDJ400, fx=context.PiFlexPadFx;
    const binding = (status,note) => {
        const matches=controls.filter(c=>c.status===status && c.note===note);
        assert.equal(matches.length,1,`unique binding for ${status.toString(16)}:${note.toString(16)}`);
        return matches[0];
    };
    const send = (status,note,value) => {
        const c=binding(status,note);
        assert.ok(c.scripted,`script binding: ${c.key}`);
        const name=c.key.split('.')[1]; assert.equal(typeof m[name],'function',c.key);
        exercised.add(`${status}:${note}`);
        m[name](status & 15,note,value,status,c.group);
    };
    const fire = id => {const t=timers.get(id); assert.ok(t); if(t.once)timers.delete(id); t.fn();};
    return {m,fx,engine,get,seed,set,writes,leds,calls,timers,listeners,binding,send,fire,
        advance:ms=>{now+=ms;},clear:()=>{writes.length=0;leds.length=0;calls.length=0;}};
}
module.exports={createHarness,controls,exercised,xml};
