#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QTest>
#include "preferences/padfxsettings.h"
#include <cmath>
#include <vector>

#include "control/controlobject.h"
#include "effects/backends/builtin/filtereffect.h"
#include "effects/effectchain.h"
#include "effects/chains/standardeffectchain.h"
#include "effects/effectparameterslotbase.h"
#include "effects/effectslot.h"
#include "effects/effectsmanager.h"
#include "engine/effects/engineeffectsmanager.h"
#include "engine/effects/groupfeaturestate.h"
#include "test/mixxxtest.h"
#include "effects/chains/padeffectchain.h"

class PadFxRoutingTest : public MixxxTest {};

// Adapted from xsploit/bitedj's 4c1dfec590 control-to-audio regression.
// Exercise our standard rack rather than importing the fork's Pad FX lanes.
TEST_F(PadFxRoutingTest, ProgrammaticEnableAndDisableReachAudio) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    manager.addDeck(deck);
    ChannelHandleAndGroup deck2(factory->getOrCreateHandle("[Channel2]"), "[Channel2]");
    manager.registerInputChannel(deck2);
    manager.addDeck(deck2);
    ControlObject loaded1(ConfigKey("[Channel1]", "track_loaded"));
    ControlObject loaded2(ConfigKey("[Channel2]", "track_loaded"));
    auto chain = manager.getEffectChain("[PadEffectRack1_[Channel1]_sweep]");
    ASSERT_TRUE(chain);
    const auto set = [&](const char* key, double value) {
        ControlObject::set(ConfigKey(chain->group(), key), value);
        QCoreApplication::processEvents();
    };
    set("param_lpf", 500);
    set("prepare", 1);
    auto* engine = manager.getEngineEffectsManager();
    QCoreApplication::processEvents();
    constexpr int frames = 256;
    std::vector<CSAMPLE> buffer(frames * 2);
    long long frame = 0;
    GroupFeatureState features;
    auto energy = [&] {
        double result = 0;
        for (int block = 0; block < 50; ++block) {
            for (int i = 0; i < frames; ++i, ++frame) {
                buffer[2 * i] = buffer[2 * i + 1] = static_cast<CSAMPLE>(
                        0.25 * std::sin(2 * 3.141592653589793 * 8000 * frame / 48000));
            }
            engine->onCallbackStart();
            engine->processPostFaderInPlace(deck.handle(), output.handle(),
                    buffer.data(), buffer.size(), mixxx::audio::SampleRate(48000),
                    features, 1, 1, false);
            if (block > 10) {
                for (auto sample : buffer) {
                    result += sample * sample;
                }
            }
        }
        return result;
    };
    const auto bypass = energy();
    ASSERT_GT(bypass, 1.0);
    set("active", 1);
    EXPECT_LT(energy(), bypass * 0.05);
    set("active", 1); // unchanged state must remain enabled
    EXPECT_LT(energy(), bypass * 0.05);
    set("active", 0);
    EXPECT_NEAR(energy(), bypass, bypass * 0.01);
    set("active", 0);
    EXPECT_NEAR(energy(), bypass, bypass * 0.01);

    // Exercise the actual touch -> system JS -> native DSP path with no MIDI.
    PadFxSettings settings(config());
    ASSERT_TRUE(settings.startPerformance(ConfigObject<ConfigValue>::computeResourcePath()));
    ControlObject::set(ConfigKey("[PadFX]", "d1_mode"), 1);
    QTest::qWait(20);
    ControlObject::set(ConfigKey("[PadFX]", "d1_touch_p1"), 1);
    QTest::qWait(20);
    EXPECT_LT(energy(), bypass * 0.8);
    ControlObject::set(ConfigKey("[PadFX]", "d1_hardware_p1"), 1);
    QTest::qWait(20);
    ControlObject::set(ConfigKey("[PadFX]", "d1_touch_p1"), 0);
    QTest::qWait(20);
    EXPECT_LT(energy(), bypass * 0.8); // hardware still owns its hold
    ControlObject::set(ConfigKey("[PadFX]", "d1_hardware_p1"), 0);
    QTest::qWait(20);
    EXPECT_NEAR(energy(), bypass, bypass * 0.01);
}
