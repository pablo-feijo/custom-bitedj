const assert = require('node:assert/strict');
const {test} = require('node:test');
const {createHarness,controls,exercised,xml} = require('./ddj400_harness.cjs');
const nativeChecked = new Set();
const rack='[EffectRack1_EffectUnit1]', slot='[EffectRack1_EffectUnit1_Effect1]';

test('14-bit filters are independent; either Shift routes Super alongside Mix',()=>{
    for(const shift of [null,0x90,0x91]) {
        const h=createHarness();
        if(shift)h.send(shift,0x3f,127);
        for(const [msb,lsb,deck] of [[0x17,0x37,1],[0x18,0x38,2]]) {
            const group=shift?rack:`[QuickEffectRack1_[Channel${deck}]]`;
            for(const [hi,lo] of [[0,0],[64,0],[85,42],[127,127]]) {
                h.send(0xb6,msb,hi);h.send(0xb6,lsb,lo);
                assert.equal(h.get(group,'super1'),((hi<<7)|lo)/16383);
            }
        }
        for(const value of [0,64,127]) {
            h.send(0xb4,0x02,value);assert.equal(h.get(rack,'mix'),value/127);
        }
        assert.ok(!h.writes.some(([,k])=>k==='meta'||k==='loaded_chain_preset'));
        if(shift) {
            h.send(shift,0x3f,0);h.send(0xb6,0x37,0);
            assert.equal(h.get('[QuickEffectRack1_[Channel1]]','super1'),16256/16383);
        }
    }
    const h=createHarness();h.send(0xb6,0x17,20);h.send(0xb6,0x18,100);
    h.send(0xb6,0x37,1);h.send(0xb6,0x38,2);
    assert.equal(h.get('[QuickEffectRack1_[Channel1]]','super1'),2561/16383);
    assert.equal(h.get('[QuickEffectRack1_[Channel2]]','super1'),12802/16383);
});

test('crossfader disable blocks hardware; enabling restores full travel',()=>{
    const h=createHarness();
    for(const v of [0,64,127])h.send(0xb6,0x1f,v);
    assert.deepEqual(h.writes,[]);
    h.seed('[BiteDJ]','crossfader_enabled',1);
    for(const v of [0,64,127]) {
        h.send(0xb6,0x1f,v);assert.equal(h.get('[Master]','crossfader'),v/63.5-1);
    }
    h.seed('[BiteDJ]','crossfader_enabled',0);h.clear();h.send(0xb6,0x1f,0);
    assert.deepEqual(h.writes,[]);
});

test('Beat FX bucket navigation selects loaded Beats parameter, clamps and ignores releases',()=>{
    const h=createHarness();h.send(0x94,0x4b,127);assert.deepEqual(h.writes,[]);
    h.seed(slot,'parameter1_loaded',1);h.seed(slot,'parameter1_units',0);
    h.seed(slot,'parameter2_units',1); // Not loaded: must skip.
    h.seed(slot,'parameter16_loaded',1);h.seed(slot,'parameter16_units',1);
    const key='parameter16_beat_period';h.seed(slot,key,.125);h.seed(slot,key+'_min',.125);h.seed(slot,key+'_max',4);
    for(const v of [.25,.5,1,2,4]) {h.send(0x94,0x4b,127);assert.equal(h.get(slot,key),v);}
    h.clear();h.send(0x94,0x4b,127);assert.deepEqual(h.writes,[]);
    for(const v of [2,1,.5,.25,.125]) {h.send(0x94,0x4a,127);assert.equal(h.get(slot,key),v);}
    h.clear();h.send(0x94,0x4a,127);h.send(0x94,0x4a,0);h.send(0x94,0x4b,0);
    assert.deepEqual(h.writes,[]);
    h.seed(slot,key,.7);h.send(0x94,0x4b,127);assert.equal(h.get(slot,key),1);
    h.seed(slot,'parameter2_loaded',1);h.seed(slot,'parameter2_beat_period',1);h.seed(slot,'parameter2_beat_period_min',.125);h.seed(slot,'parameter2_beat_period_max',4);
    h.send(0x94,0x4a,127);assert.equal(h.get(slot,'parameter2_beat_period'),.5);
    assert.equal(h.get(slot,key),1); // First matching parameter only.
});

test('Beat FX routing, enable LEDs, panic and relative selection',()=>{
    const h=createHarness();h.m.init();h.clear();
    for(const [note,a,b] of [[0x10,1,0],[0x11,0,1],[0x14,1,1]]) {
        h.send(0x94,note,127);
        assert.deepEqual(h.writes.splice(0),[[rack,'group_[Channel1]_enable',a],
            [rack,'group_[Channel2]_enable',b],[rack,'group_[Master]_enable',0]]);
        h.send(0x94,note,0);assert.deepEqual(h.writes,[]);
    }
    h.send(0x94,0x47,127);assert.equal(h.get(slot,'enabled'),true);
    assert.deepEqual(h.leds.slice(-2),[[0x94,0x47,127],[0x94,0x43,127]]);
    h.send(0x94,0x47,0);assert.equal(h.get(slot,'enabled'),true);
    h.send(0x94,0x47,127);assert.equal(h.get(slot,'enabled'),false);
    for(let i=1;i<=3;i++)h.seed(`[EffectRack1_EffectUnit1_Effect${i}]`,'enabled',1);
    h.seed(rack,'mix',.8);h.send(0x94,0x43,127);
    for(let i=1;i<=3;i++)assert.equal(h.get(`[EffectRack1_EffectUnit1_Effect${i}]`,'enabled'),0);
    assert.equal(h.get(rack,'mix'),0);
    assert.ok(h.calls.some(c=>c[0]==='softTakeoverIgnoreNextValue'&&c[2]==='mix'));
    h.clear();h.send(0x94,0x43,0);assert.deepEqual(h.writes,[]);
    for(const [shift,expected] of [[null,1],[0x90,-1],[0x91,-1]]) {
        if(shift)h.send(shift,0x3f,127);
        h.send(0x94,0x63,127);assert.equal(h.get(rack,'chain_selector'),expected);
        if(shift)h.send(shift,0x3f,0);
    }
    h.send(0x94,0x64,127);assert.equal(h.get(rack,'chain_selector'),-1);
    h.clear();h.send(0x94,0x63,0);h.send(0x94,0x64,0);assert.deepEqual(h.writes,[]);
});

test('Browse zoom on Play, signed library movement elsewhere and Shift tab toggle',()=>{
    const h=createHarness();
    for(const [value,key] of [[127,'waveform_zoom_up'],[1,'waveform_zoom_down']]) {
        h.send(0xb6,0x40,value);assert.deepEqual(h.calls.at(-1),['triggerControl','[Channel1]',key,100]);
    }
    for(const tab of [1,2,3,4]) {
        h.seed('[Tab]','current',tab);
        for(const [v,expected] of [[1,1],[127,-1],[2,2],[126,-2]]) {
            h.send(0xb6,0x40,v);assert.equal(h.get('[Library]','MoveVertical'),expected);
        }
        h.send(0x96,0x42,127);assert.equal(h.get('[Tab]','current'),tab===1?0:1);
        h.clear();h.send(0x96,0x42,0);assert.deepEqual(h.writes,[]);
    }
});

test('Load and Instant Doubles: both directions, timing boundary, releases and independent clocks',()=>{
    for(const deck of [1,2]) for(const playing of [0,1]) for(const synced of [0,1]) {
        const h=createHarness(), group=`[Channel${deck}]`, other=`[Channel${3-deck}]`, note=0x45+deck;
        h.seed(other,'playposition',.375);h.seed(other,'play',playing);h.seed(other,'sync_enabled',synced);
        h.send(0x96,note,127);assert.deepEqual(h.writes,[[group,'LoadSelectedTrack',1]]);
        h.clear();h.send(0x96,note,0);assert.deepEqual(h.writes,[]);
        h.advance(499);h.send(0x96,note,127);
        assert.deepEqual(h.writes,[[group,'CloneFromDeck',3-deck],[group,'playposition',.375],
            ...(synced?[[group,'sync_enabled',1]]:[]),[group,'beatsync_phase',1],
            [group,'beatsync_phase',0],[group,'play',playing]]);
        h.clear();h.advance(500);h.send(0x96,note,127);
        assert.deepEqual(h.writes,[[group,'LoadSelectedTrack',1]]);
    }
    const h=createHarness();h.send(0x96,0x46,127);h.advance(100);h.send(0x96,0x47,127);
    assert.deepEqual(h.writes,[['[Channel1]','LoadSelectedTrack',1],['[Channel2]','LoadSelectedTrack',1]]);
});

test('Sync short/long press and tempo controls remain independent per deck',()=>{
    const h=createHarness();
    for(const deck of [1,2]) {
        const status=0x8f+deck,g=`[Channel${deck}]`;
        h.send(status,0x58,127);assert.equal(h.get(g,'sync_enabled'),true);
        h.send(status,0x58,0);assert.equal(h.get(g,'sync_enabled'),true);
        h.send(status,0x58,127);assert.equal(h.get(g,'sync_enabled'),false);
        h.send(status,0x5c,127);h.send(status,0x5c,127);assert.equal(h.get(g,'sync_enabled'),1);
        h.clear();h.send(status,0x5c,0);h.send(status,0x60,0);assert.deepEqual(h.writes,[]);
        h.seed(g,'rateRange',.06);
        for(const range of [.10,.25,.50,1,.06]) {h.send(status,0x60,127);assert.equal(h.get(g,'rateRange'),range);}
        for(const [hi,lo] of [[0,0],[64,0],[127,127]]) {
            h.send(status+0x20,0x00,hi);h.send(status+0x20,0x20,lo);
            assert.equal(h.get(g,'rate'),1-((hi<<7)+lo)/8192);
        }
    }
});

test('Pad FX: every XML pad binding reaches the native adapter and preserves release identity',()=>{
    assert.ok(xml.includes('filename="piflex-padfx.js"'));
    const h=createHarness();h.fx.init(2);
    for(const deck of [1,2]) {
        const g=`[Channel${deck}]`;
        h.seed(g,'play',1);h.seed(g,'bpm',128);
        for(const lane of h.fx.lanes)h.seed(h.fx.group(g,lane),'available',1);
        for(const shifted of [false,true]) for(let pad=0;pad<8;pad++) {
            const status=0x95+deck*2+(shifted?1:0),note=(shifted?0x60:0x10)+pad;
            h.send(status,note,127);
            assert.equal(Object.keys(h.fx.decks[g].held).length,1,`deck ${deck}, bank ${shifted}, pad ${pad}`);
            assert.deepEqual(h.leds.at(-1),[status,note,127]);
            h.send(status,note,127);assert.equal(Object.keys(h.fx.decks[g].held).length,1);
            h.send(status-0x10,note,64); // Real NoteOff with nonzero velocity.
            assert.equal(Object.keys(h.fx.decks[g].held).length,0);
            assert.deepEqual(h.leds.at(-1),[status,note,0]);
            h.send(status,note,127);h.send(status,note,0); // Zero-velocity NoteOn release.
            assert.equal(Object.keys(h.fx.decks[g].held).length,0);
        }
        const normal=0x95+deck*2;
        h.send(normal,0x14,127);h.send(normal+1,0x64,0);
        assert.equal(Object.keys(h.fx.decks[g].held).length,0,'Shift change releases original pad');
    }
    assert.ok(!h.writes.some(([g,k])=>g===rack || k==='volume' || k==='play'));
    h.fx.shutdown();assert.equal(h.timers.size,0);
});

test('Held Pad FX does not intercept Mix, Super or Sync',()=>{
    const h=createHarness();h.m.init();
    for(const deck of [1,2]) {
        const g=`[Channel${deck}]`,s=0x8f+deck;
        h.seed(g,'play',1);h.seed(g,'bpm',128);
        for(const lane of h.fx.lanes)h.seed(h.fx.group(g,lane),'available',1);
        h.send(0x95+2*deck,0x14,127);
        h.send(s,0x3f,127);h.send(0xb6,0x16+deck,127);h.send(0xb6,0x36+deck,127);
        h.send(0xb4,0x02,64);h.send(s,0x58,127);
        assert.equal(h.get(rack,'super1'),1);assert.equal(h.get(rack,'mix'),64/127);
        assert.equal(h.get(g,'sync_enabled'),true);
        assert.equal(Object.keys(h.fx.decks[g].held).length,1);
        h.send(s,0x3f,0);h.send(0x95+2*deck,0x14,0);
    }
    h.m.shutdown();
});

test('All controller modes and triple Shift dispatch through their XML bindings',()=>{
    const h=createHarness();
    for(const deck of [1,2]) {
        const status=0x8f+deck;
        for(const [note,mode] of [[0x1b,0],[0x1e,1],[0x20,2],[0x6d,3]]) {
            h.send(status,0x3f,127);h.send(status,note,127);h.send(status,0x3f,0);
            assert.equal(h.get('[Skin]','cue_panel'),deck);
            assert.equal(h.get('[PadFX]',`d${deck}_mode`),mode);
            h.send(status,note,0);assert.equal(h.get('[Skin]','cue_panel'),deck);
            for(const side of [0x90,0x91]) {
                h.send(side,0x3f,127);h.send(side,0x3f,0);h.advance(100);
                assert.equal(h.get('[Skin]','cue_panel'),deck);
            }
            h.send(status,0x3f,127);h.send(status,0x3f,0);
            assert.equal(h.get('[Skin]','cue_panel'),0);
            assert.equal(h.get('[PadFX]',`d${deck}_mode`),mode);
        }
        for(const note of [0x69,0x6b,0x22,0x6f]) {
            h.send(status,0x1e,127);h.send(status,note,127);
            assert.equal(h.get('[Skin]','cue_panel'),0);
            h.send(0x92-deck,0x1e,127);h.send(status,note,127);
            assert.equal(h.get('[Skin]','cue_panel'),3-deck);
        }
    }
});

test('Beat Jump pads in all three banks, bank limits, loop scale, quick jump and quantize',()=>{
    const h=createHarness();
    for(const deck of [1,2]) {
        const g=`[Channel${deck}]`,status=0x95+2*deck,deckStatus=0x8f+deck;
        h.send(status+1,0x26,127);h.send(status+1,0x26,127);
        for(const multiplier of [1/16,1,16]) {
            for(const d of [1,2])assert.equal(h.get('[PadFX]',`d${d}_jump_bank`),multiplier<1?0:multiplier>1?2:1);
            for(const [pad,size] of [-1,1,-2,2,-4,4,-8,8].entries()) {
                h.clear();h.send(status,0x20+pad,127);
                assert.deepEqual(h.writes,[[g,'beatjump_size',Math.abs(size*multiplier)],[g,'beatjump',size*multiplier]]);
                h.clear();h.send(status,0x20+pad,0);assert.deepEqual(h.writes,[]);
            }
            h.send(status+1,0x27,127);
        }
        assert.equal(h.get('[PadFX]','d1_jump_bank'),2);
        h.clear();h.send(status+1,0x27,0);h.send(status+1,0x26,0);assert.deepEqual(h.writes,[]);
        for(const [note,key,value] of [[0x51,'loop_scale',.5],[0x53,'loop_scale',2],
            [0x3e,'beatjump',-32],[0x3d,'beatjump',32]]) {
            h.clear();h.send(deckStatus,note,127);assert.deepEqual(h.writes,[[g,key,value]]);
            h.clear();h.send(deckStatus,note,0);assert.deepEqual(h.writes,[]);
        }
        h.send(deckStatus,0x68,127);assert.equal(h.get(g,'quantize'),1);
        h.send(deckStatus,0x68,0);assert.equal(h.get(g,'quantize'),1);
        h.send(deckStatus,0x68,127);assert.equal(h.get(g,'quantize'),0);
    }
});

test('Settings vinyl mode controls scratching; jog bend, grid alignment and loop adjustment on both decks',()=>{
    const h=createHarness();h.seed('[BiteDJ]','vinyl_mode',0);h.m.init();h.clear();
    for(const deck of [1,2]) {
        const s=0x8f+deck,g=`[Channel${deck}]`;
        for(const touch of [0x36]) {
            h.set('[BiteDJ]','vinyl_mode',0);h.send(s,touch,127);
            assert.deepEqual(h.calls.at(-1),['scratchDisable',deck,false]);
            h.set('[BiteDJ]','vinyl_mode',1);h.send(s,touch,127);
            assert.equal(h.calls.at(-1)[0],'scratchEnable');assert.equal(h.calls.at(-1)[1],deck);
            h.send(s,touch,0);assert.deepEqual(h.calls.at(-1),['scratchDisable',deck,false]);
        }
        for(const jog of [0x21,0x22,0x23]) {
            for(const value of [63,64,65]) {
                h.send(s+0x20,jog,value);assert.equal(h.get(g,'jog'),(value-64)*.8);
            }
            h.seed(g,'test_scratching',1);h.send(s+0x20,jog,66);
            assert.deepEqual(h.calls.at(-1),['scratchTick',deck,2]);h.seed(g,'test_scratching',0);
        }
        h.send(s+0x20,0x29,63);assert.equal(h.get(g,'beats_translate_move'),-1);
        h.seed(g,'loop_enabled',1);h.seed(g,'loop_start_position',100);h.seed(g,'loop_end_position',500);
        h.send(s,0x4c,127);h.send(s,0x4c,0);h.send(s+0x20,0x22,65);
        assert.equal(h.get(g,'loop_start_position'),150);assert.equal(h.get(g,'loop_end_position'),500);
        h.clear();h.send(s,0x36,127);assert.deepEqual(h.calls,[]);
        h.send(s,0x4e,127);h.send(s,0x4e,0);h.send(s+0x20,0x22,63);
        assert.equal(h.get(g,'loop_end_position'),450);assert.equal(h.get(g,'loop_start_position'),150);
        h.set(g,'loop_enabled',0);assert.equal(h.m.loopAdjustIn[deck-1],false);assert.equal(h.m.loopAdjustOut[deck-1],false);
    }
});

test('Sampler load/play/stop/eject on all 16 pads and paired Shift LEDs',()=>{
    const h=createHarness();h.m.init();
    for(let sampler=1;sampler<=16;sampler++) {
        const status=sampler<=8?0x97:0x99,note=0x30+(sampler-1)%8,g=`[Sampler${sampler}]`;
        h.clear();h.send(status,note,127);h.send(status,note,0);
        assert.deepEqual(h.writes,[[g,'LoadSelectedTrack',127],[g,'LoadSelectedTrack',0]]);
        h.seed(g,'track_loaded',1);h.clear();h.send(status,note,127);h.send(status,note,0);
        assert.deepEqual(h.writes,[[g,'cue_gotoandplay',127],[g,'cue_gotoandplay',0]]);
        h.set(g,'play',1);const timer=[...h.timers.keys()].at(-1);h.fire(timer);
        assert.deepEqual(h.leds.slice(-2),[[status,note,0],[status+1,note,0]]);
        h.clear();h.send(status+1,note,127);h.send(status+1,note,0);
        assert.deepEqual(h.writes,[[g,'cue_gotoandstop',127],[g,'cue_gotoandstop',0]]);
        h.set(g,'play',0);h.fire(timer);assert.ok(!h.timers.has(timer));
        assert.deepEqual(h.leds.slice(-2),[[status,note,127],[status+1,note,127]]);
        h.clear();h.send(status+1,note,127);h.send(status+1,note,0);
        assert.deepEqual(h.writes,[[g,'eject',127],[g,'eject',0]]);
        h.seed(g,'track_loaded',0);h.clear();h.send(status+1,note,127);assert.deepEqual(h.writes,[]);
    }
});

test('Startup, track/VU feedback, pending loop LEDs and shutdown Pad FX cleanup',()=>{
    const h=createHarness();h.m.init();
    assert.equal(h.get('[App]','num_samplers'),16);
    assert.deepEqual(Object.keys(h.fx.decks),['[Channel1]','[Channel2]']);
    assert.ok(h.calls.some(c=>c[0]==='sysex'&&c[2]===12));
    for(const deck of [1,2]) {
        const s=0x8f+deck,g=`[Channel${deck}]`;
        h.set(g,'track_loaded',1);assert.deepEqual(h.leds.at(-1),[0x9f,deck-1,127]);
        h.set(g,'track_loaded',0);assert.deepEqual(h.leds.at(-1),[0x9f,deck-1,0]);
        h.set(g,'vu_meter',.5);assert.deepEqual(h.leds.at(-1),[0xaf+deck,0x02,75]);
        h.seed(g,'loop_end_position',-1);h.set(g,'loop_start_position',100);
        let timer=h.m.timers[g].loopInPending;h.fire(timer);
        assert.deepEqual(h.leds.slice(-2),[[s,0x10,0],[s,0x4c,0]]);
        h.set(g,'loop_enabled',1);assert.ok(!h.timers.has(timer));
        timer=h.m.timers[g].loop_enabled;h.fire(timer);
        assert.deepEqual(h.leds.slice(-4),[[s,0x10,0],[s,0x4c,0],[s,0x11,0],[s,0x4e,0]]);
        h.set(g,'loop_enabled',0);assert.ok(!h.timers.has(timer));
        h.seed(g,'play',1);h.seed(g,'bpm',128);
        for(const lane of h.fx.lanes)h.seed(h.fx.group(g,lane),'available',1);
        h.send(0x95+deck*2,0x14,127);
    }
    h.m.shutdown();assert.equal(h.timers.size,0);
    for(const deck of [1,2])assert.equal(h.get('[PadFX]',`d${deck}_shift`),0);
    assert.ok(h.leds.slice(-10).every(([, , value])=>value===0));
});

test('Native hotcue, loop, transport and mixer bindings retain their targets',()=>{
    const h=createHarness();
    const expect=(s,n,g,k)=>{const c=h.binding(s,n);assert.equal(c.scripted,false);assert.equal(c.group,g);assert.equal(c.key,k);nativeChecked.add(`${s}:${n}`);
        if ((s & 0xf0) === 0xb0) {
            const half=n>=0x20?'fourteen-bit-lsb':'fourteen-bit-msb';
            assert.deepEqual(c.options,[half,...(s===0xb6?[]:['soft-takeover'])]);
        } else assert.deepEqual(c.options,['normal']);
    };
    for(const deck of [1,2]) {
        const s=0x8f+deck,p=0x95+deck*2,g=`[Channel${deck}]`;
        for(const [note,key] of [[0x0b,'play'],[0x47,'reverseroll'],[0x0c,'cue_default'],[0x48,'start_play'],
            [0x10,'loop_in'],[0x11,'loop_out'],[0x4d,'reloop_toggle'],[0x50,'reloop_andstop'],[0x54,'pfl']])expect(s,note,g,key);
        for(let i=0;i<8;i++) {
            expect(p,i,g,`hotcue_${i+1}_activate`);expect(p+1,i,g,`hotcue_${i+1}_clear`);
            expect(p,0x60+i,g,i<4?`beatlooproll_${[.25,.5,1,2][i]}_activate`:`beatloop_${[4,8,16,32][i-4]}_toggle`);
        }
        for(const [note,key] of [[0x33,'volume'],[0x13,'volume'],[0x24,'pregain'],[0x04,'pregain']])expect(s+0x20,note,g,key);
        for(const [note,param] of [[0x27,3],[0x2b,2],[0x2f,1],[0x07,3],[0x0b,2],[0x0f,1]])expect(s+0x20,note,`[EqualizerRack1_[Channel${deck}]_Effect1]`,`parameter${param}`);
    }
    expect(0x96,0x41,'[Library]','MoveFocusForward');
    for(const [note,key] of [[0x2c,'headMix'],[0x0c,'headMix'],[0x2d,'headGain'],[0x0d,'headGain']])expect(0xb6,note,'[Master]',key);
});

test('Shift jog only translates grid, including shifted touch and mid-touch Shift',()=>{
    for (const deck of [1,2]) {
        const h=createHarness(), s=0x8f+deck, g=`[Channel${deck}]`;
        h.m.vinylMode=true;
        h.send(s,0x67,127); assert.deepEqual(h.calls,[]);
        h.send(s,0x67,0); assert.deepEqual(h.calls,[['scratchDisable',deck,false]]);
        for (const playing of [0,1]) {
            h.seed(g,'play',playing); h.clear();
            h.send(s,0x36,127);
            h.send(s,0x3f,127);
            assert.ok(h.calls.some(c=>c[0]==='scratchDisable' && c[2]===false));
            h.clear();
            for (const note of [0x21,0x22,0x23,0x29]) {
                h.send(s+0x20,note,63); h.send(s+0x20,note,64); h.send(s+0x20,note,66);
            }
            assert.deepEqual(h.writes,Array.from({length:4},()=>[
                [g,'beats_translate_move',-1],[g,'beats_translate_move',2]]).flat());
            assert.deepEqual(h.calls,[]);
            h.send(s,0x67,0); h.send(s,0x3f,0);
            assert.equal(h.get(g,'play'),playing);
        }
    }
});

test('Jog release resumes playing decks immediately and leaves paused decks paused',()=>{
    for (const deck of [1,2]) for (const playing of [0,1]) {
        const h=createHarness(), s=0x8f+deck, g=`[Channel${deck}]`;
        h.m.vinylMode=true; h.seed(g,'play',playing);
        h.send(s,0x36,127);
        // A loop adjustment starting during touch must never swallow release.
        h.m.loopAdjustIn[deck-1]=true; h.clear(); h.send(s,0x36,0);
        assert.deepEqual(h.calls,[['scratchDisable',deck,false]]);
        assert.deepEqual(h.writes,playing?[[g,'play',1]]:[]);
    }
});

test('Shift Browse zoom and focused FX follow either deck Shift and all three slots',()=>{
    const h=createHarness(), unit='[EffectRack1_EffectUnit1]'; h.m.init();
    for (const shift of [0x90,0x91]) {
        h.seed('[Tab]','current',1); h.send(shift,0x3f,127); h.clear();
        h.send(0xb6,0x40,127);
        assert.deepEqual(h.calls,[['triggerControl','[Channel1]','waveform_zoom_up',100]]);
        for (let slot=1;slot<=3;slot++) h.seed(`[EffectRack1_EffectUnit1_Effect${slot}]`,'enabled',1);
        h.m.beatFxOnOffPressed(4,0,127);
        for (let slot=1;slot<=3;slot++) assert.equal(h.get(`[EffectRack1_EffectUnit1_Effect${slot}]`,'enabled'),0);
        h.send(shift,0x3f,0);
    }
    for (const slot of [1,2,3]) {
        const g=`[EffectRack1_EffectUnit1_Effect${slot}]`;
        h.set(unit,'focused_effect',slot); h.seed(g,'enabled',0); h.clear();
        h.m.beatFxOnOffPressed(4,0,127); assert.equal(h.get(g,'enabled'),true);
        h.seed(g,'parameter1_loaded',1);h.seed(g,'parameter1_units',1);h.seed(g,'parameter1_beat_period',1);h.seed(g,'parameter1_beat_period_min',.125);h.seed(g,'parameter1_beat_period_max',4);
        h.m.beatFxRightPressed(4,0,127); assert.equal(h.get(g,'parameter1_beat_period'),2);
        h.m.beatFxLeftPressed(4,0,127); assert.equal(h.get(g,'parameter1_beat_period'),1);
    }
});

test('Active-loop jog resizes in measured steps without scratch or pitch bend',()=>{
    for (const deck of [1,2]) for (const playing of [0,1]) {
        const h=createHarness(), status=0x8f+deck, g=`[Channel${deck}]`;
        h.m.init(); h.m.vinylMode=true; h.seed(g,'play',playing); h.set(g,'loop_enabled',1);
        h.clear(); h.send(status,0x36,127); assert.deepEqual(h.calls,[]);
        for (const note of [0x21,0x22,0x23]) {
            h.m.loopJogTicks[deck-1]=0; h.clear();
            h.send(status+0x20,note,95); assert.deepEqual(h.writes,[]);
            h.send(status+0x20,note,65); assert.deepEqual(h.writes,[[g,'loop_scale',2]]);
            h.send(status+0x20,note,32); assert.deepEqual(h.writes.at(-1),[g,'loop_scale',0.5]);
            assert.deepEqual(h.calls,[]);
        }
        // Reversing direction discards the partial turn in the old direction.
        h.clear();h.send(status+0x20,0x22,95);h.send(status+0x20,0x22,32);
        assert.deepEqual(h.writes,[[g,'loop_scale',0.5]]);
        h.send(status,0x3f,127);h.clear();h.send(status+0x20,0x29,96);
        assert.deepEqual(h.writes,[[g,'beats_translate_move',32]]);
        h.send(status,0x3f,0);h.set(g,'loop_enabled',0);h.clear();
        h.send(status+0x20,0x22,65);assert.deepEqual(h.writes,[[g,'jog',0.8]]);
        assert.equal(h.get(g,'play'),playing);
        const other=deck===1?1:0;assert.equal(h.m.loopJogTicks[other],0);
    }
});

test('Enabling a loop during scratch releases immediately and invalid loop edit is ignored',()=>{
    const h=createHarness();h.m.init();h.m.vinylMode=true;
    for (const deck of [1,2]) {
        const status=0x8f+deck,g=`[Channel${deck}]`;
        h.seed(g,'loop_enabled',0);h.seed(g,'play',1);
        h.send(status,0x4c,127);h.send(status,0x4e,127);
        assert.equal(h.m.loopAdjustIn[deck-1],false);assert.equal(h.m.loopAdjustOut[deck-1],false);
        h.send(status,0x36,127);h.clear();h.set(g,'loop_enabled',1);
        assert.ok(h.calls.some(c=>c[0]==='scratchDisable' && c[1]===deck && c[2]===false));
        assert.equal(h.get(g,'play'),1);h.clear();
        h.send(status+0x20,0x22,95);h.send(status,0x36,0);h.clear();
        h.send(status+0x20,0x22,65);assert.deepEqual(h.writes,[]);
    }
});

test('Mapping audit: unique MIDI inputs, resolvable callbacks and every script binding exercised',()=>{
    const h=createHarness(), seen=new Set();
    for(const c of controls) {
        const key=`${c.status}:${c.note}`;assert.ok(!seen.has(key),`duplicate ${key}`);seen.add(key);
        if(c.scripted) {
            assert.equal(typeof h.m[c.key.split('.')[1]],'function',c.key);
            assert.ok(exercised.has(key),`Missing behavioral coverage for ${c.key} (${key})`);
        } else assert.ok(nativeChecked.has(key),`Missing native binding contract for ${c.key} (${key})`);
    }
    console.log(`Audited ${controls.length} MIDI inputs; exercised ${exercised.size} script bindings.`);
});
