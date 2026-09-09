#include <gtest/gtest.h>

#include <QCoreApplication>
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

class EffectSlotTest : public MixxxTest {};

// Adapted from xsploit/bitedj's 4c1dfec590 control-to-audio regression.
// Exercise our standard rack rather than importing the fork's Pad FX lanes.
TEST_F(EffectSlotTest, ProgrammaticEnableAndDisableReachAudio) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    auto chain = manager.getStandardEffectChain(0);
    ASSERT_TRUE(chain);
    auto slot = chain->getEffectSlot(0);
    ASSERT_TRUE(slot);
    const auto manifest = manager.getBackendManager()->getManifest(
            FilterEffect::getId(), EffectBackendType::BuiltIn);
    ASSERT_TRUE(manifest);
    slot->loadEffectWithDefaults(manifest);
    ASSERT_TRUE(slot->isLoaded());
    ControlObject::set(ConfigKey(StandardEffectChain::formatEffectSlotGroup(0, 0),
                               "parameter1_value"), 500.0);
    ControlObject::set(ConfigKey(chain->group(), "group_[Channel1]_enable"), 1.0);
    ControlObject::set(ConfigKey(chain->group(), "mix"), 1.0);
    // Start disabled through an external writer, then test the C++ setter alone.
    ControlObject::set(ConfigKey(StandardEffectChain::formatEffectSlotGroup(0, 0), "enabled"), 0.0);
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
    slot->setEnabled(true);
    EXPECT_LT(energy(), bypass * 0.05);
    slot->setEnabled(true); // unchanged state must remain enabled
    EXPECT_LT(energy(), bypass * 0.05);
    slot->setEnabled(false);
    EXPECT_NEAR(energy(), bypass, bypass * 0.01);
    slot->setEnabled(false);
    EXPECT_NEAR(energy(), bypass, bypass * 0.01);
    ControlObject::set(ConfigKey(StandardEffectChain::formatEffectSlotGroup(0, 0), "enabled"), 1.0);
    EXPECT_LT(energy(), bypass * 0.05);
}
