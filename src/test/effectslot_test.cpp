#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QFile>
#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QTimer>
#include <QSet>
#include "widget/weffectchainpresetselector.h"
#include "skin/legacy/skincontext.h"
#include <cmath>
#include <vector>

#include "control/controlobject.h"
#include "effects/backends/builtin/filtereffect.h"
#include "effects/effectchain.h"
#include "effects/chains/standardeffectchain.h"
#include "effects/effectparameterslotbase.h"
#include "effects/effectslot.h"
#include "effects/effectsmanager.h"
#include "effects/presets/effectchainpreset.h"
#include "effects/presets/effectpreset.h"
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


TEST_F(EffectSlotTest, RekordboxCatalogueLoadsValidDistinctPresetsAndGroupedActivation) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    auto chain = manager.getStandardEffectChain(0);
    auto presets = manager.getChainPresetManager();
    const QStringList expected = QStringLiteral("DELAY|ECHO|SPIRAL|REVERB|REV DELAY|MT DELAY|PITCH ECHO|TRANS|PAN|FILTER|FLANGER|PHASER|SLIP ROLL|ROLL|REV ROLL|ROBOT|PITCH|ENIGMA JET|MOBIUS SAW|MOBIUS TRI|LOW CUT ECHO|PING PONG|HELIX|VINYL BRAKE|STRETCH").split('|');
    const auto sorted = presets->getPresetsSorted();
    ASSERT_GE(sorted.size(), expected.size() + 1);
    EXPECT_EQ(sorted[0]->name(), kNoEffectString);
    ControlObject::set(ConfigKey(chain->group(), "group_[Channel1]_enable"), 1.0);
    ControlObject::set(ConfigKey(chain->group(), "mix"), 1.0);
    auto* engine = manager.getEngineEffectsManager();
    constexpr int frames = 256;
    std::vector<CSAMPLE> buffer(frames * 2);
    GroupFeatureState features;
    for (int index = 0; index < expected.size(); ++index) {
        const auto preset = sorted[index + 1];
        SCOPED_TRACE(expected[index].toStdString());
        EXPECT_EQ(preset->displayName(), expected[index]);
        EXPECT_TRUE(preset->isReadOnly());
        EXPECT_FALSE(preset->description().isEmpty());
        ASSERT_LE(preset->effectPresets().size(), 3);
        for (const auto& effect : preset->effectPresets()) {
            auto manifest = manager.getBackendManager()->getManifest(effect);
            ASSERT_TRUE(manifest);
            for (const auto& parameter : effect->getParameterPresets()) {
                EffectManifestParameterPointer found;
                for (const auto& candidate : manifest->parameters()) {
                    if (candidate->id() == parameter.id()) {
                        found = candidate;
                        break;
                    }
                }
                ASSERT_TRUE(found) << parameter.id().toStdString();
                EXPECT_GE(parameter.value(), found->getMinimum());
                EXPECT_LE(parameter.value(), found->getMaximum());
            }
        }
        chain->loadChainPreset(preset);
        QCoreApplication::processEvents();
        for (int slot = 0; slot < preset->effectPresets().size(); ++slot) {
            EXPECT_TRUE(chain->getEffectSlot(slot)->isLoaded());
            EXPECT_EQ(ControlObject::get(ConfigKey(chain->getEffectSlot(slot)->getGroup(), "enabled")), 0);
        }
        ControlObject::set(ConfigKey(chain->getEffectSlot(0)->getGroup(), "enabled"), 1);
        QCoreApplication::processEvents();
        for (int slot = 0; slot < preset->effectPresets().size(); ++slot) {
            EXPECT_EQ(ControlObject::get(ConfigKey(chain->getEffectSlot(slot)->getGroup(), "enabled")), 1);
        }
        double energy = 0;
        double rightEnergy = 0;
        const bool pingPong = expected[index] == QStringLiteral("PING PONG");
        for (int block = 0; block < 400; ++block) {
            for (int i = 0; i < frames; ++i) {
                auto sample = static_cast<CSAMPLE>(0.1 * std::sin(
                        2 * 3.141592653589793 * 440 * (block * frames + i) / 48000));
                buffer[2 * i] = sample;
                buffer[2 * i + 1] = pingPong ? 0.0f : sample * 0.7f;
            }
            engine->onCallbackStart();
            engine->processPostFaderInPlace(deck.handle(), output.handle(),
                    buffer.data(), buffer.size(), mixxx::audio::SampleRate(48000),
                    features, 1, 1, false);
            for (int i = 1; i < frames * 2; i += 2) {
                rightEnergy += buffer[i] * buffer[i];
            }
            for (auto sample : buffer) {
                ASSERT_TRUE(std::isfinite(sample));
                EXPECT_LT(std::abs(sample), 4.0);
                energy += sample * sample;
            }
        }
        EXPECT_GT(energy, 0.01);
        if (pingPong) {
            EXPECT_GT(rightEnergy, 0.01) << "Left-only input must reach the right echo channel";
        }
        ControlObject::set(ConfigKey(chain->getEffectSlot(0)->getGroup(), "enabled"), 0);
        QCoreApplication::processEvents();
        for (int slot = 0; slot < preset->effectPresets().size(); ++slot) {
            EXPECT_EQ(ControlObject::get(ConfigKey(chain->getEffectSlot(slot)->getGroup(), "enabled")), 0);
        }
    }
    chain->loadChainPreset(sorted[1]);
    ControlObject::set(ConfigKey(chain->group(), "chain_selector"), 1);
    EXPECT_EQ(chain->presetName(), sorted[2]->name());
    ControlObject::set(ConfigKey(chain->group(), "chain_selector"), -1);
    EXPECT_EQ(chain->presetName(), sorted[1]->name());
    ControlObject::set(ConfigKey(chain->group(), "chain_selector"), -1);
    EXPECT_EQ(chain->presetName(), kNoEffectString);
    ControlObject::set(ConfigKey(chain->group(), "chain_selector"), -1);
    EXPECT_EQ(chain->presetName(), sorted.last()->name());
}

TEST_F(EffectSlotTest, BeatPeriodAliasMatchesEchoAndTremoloAndClampsTruthfully) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    auto chain = manager.getStandardEffectChain(0);
    auto presets = manager.getChainPresetManager();
    for (const auto& name : {QStringLiteral("[RB7] ECHO"), QStringLiteral("[RB7] TRANS")}) {
        auto preset = presets->getPreset(name);
        ASSERT_TRUE(preset);
        chain->loadChainPreset(preset);
        const auto group = chain->getEffectSlot(0)->getGroup();
        int beatSlot = 0;
        for (int i = 1; i <= 16; ++i) {
            if (ControlObject::get(ConfigKey(group, QString("parameter%1_units").arg(i))) == 1) {
                beatSlot = i;
                break;
            }
        }
        ASSERT_GT(beatSlot, 0);
        const auto prefix = QString("parameter%1").arg(beatSlot);
        ControlObject::set(ConfigKey(group, prefix + "_beat_period"), 0.125);
        EXPECT_DOUBLE_EQ(ControlObject::get(ConfigKey(group, prefix + "_value")),
                name.endsWith("TRANS") ? 8.0 : 0.125);
        EXPECT_DOUBLE_EQ(ControlObject::get(ConfigKey(group, prefix + "_beat_period")), 0.125);
        ControlObject::set(ConfigKey(group, prefix + "_beat_period"), 4.0);
        EXPECT_DOUBLE_EQ(ControlObject::get(ConfigKey(group, prefix + "_beat_period")),
                name.endsWith("TRANS") ? 4.0 : 2.0);
    }
}

TEST_F(EffectSlotTest, CatalogueUpgradePreservesSavedFilesAndReplacesStaleFactoryCopy) {
    const auto directory = QDir(config()->getSettingsPath()).filePath("effects/chains/");
    ASSERT_TRUE(QDir().mkpath(directory));
    QFile source(config()->getResourcePath() + "effects/rekordbox7/02_ECHO.xml");
    ASSERT_TRUE(source.open(QIODevice::ReadOnly));
    const auto original = source.readAll();
    auto custom = original;
    custom.replace("[RB7] ECHO", "My Live Echo");
    QFile customFile(directory + "custom.xml");
    ASSERT_TRUE(customFile.open(QIODevice::WriteOnly));
    customFile.write(custom);
    customFile.close();
    auto stale = original;
    stale.replace("<Value>0.6</Value>", "<Value>0.1</Value>");
    QFile staleFile(directory + "stale-factory.xml");
    ASSERT_TRUE(staleFile.open(QIODevice::WriteOnly));
    staleFile.write(stale);
    staleFile.close();
    // Exercise a saved old list, and repeat startup to catch accumulating entries.
    QFile state(QDir(config()->getSettingsPath()).filePath("effects.xml"));
    ASSERT_TRUE(state.open(QIODevice::WriteOnly));
    state.write("<MixxxEffects><ChainPresetList><ChainPresetName>My Live Echo</ChainPresetName>"
                "<ChainPresetName>[RB7] ECHO</ChainPresetName></ChainPresetList></MixxxEffects>");
    state.close();
    for (int startup = 0; startup < 2; ++startup) {
        auto factory = std::make_shared<ChannelHandleFactory>();
        EffectsManager manager(config(), factory);
        ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
        ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
        manager.registerOutputChannel(output);
        manager.registerInputChannel(output);
        manager.registerInputChannel(deck);
        manager.setup();
        const auto presets = manager.getChainPresetManager();
        int factoryCount = 0;
        for (const auto& preset : presets->getPresetsSorted()) {
            factoryCount += preset->isRekordbox7();
        }
        EXPECT_EQ(factoryCount, 25);
        EXPECT_EQ(presets->getPresetsSorted()[1]->name(), "[RB7] DELAY");
        ASSERT_TRUE(presets->getPreset("My Live Echo"));
        const auto echo = presets->getPreset("[RB7] ECHO");
        ASSERT_TRUE(echo);
        bool checked = false;
        for (const auto& parameter : echo->effectPresets()[0]->getParameterPresets()) {
            if (parameter.id() == "feedback_amount") {
                EXPECT_DOUBLE_EQ(parameter.value(), 0.6);
                checked = true;
            }
        }
        EXPECT_TRUE(checked);
    }
    ASSERT_TRUE(customFile.open(QIODevice::ReadOnly));
    EXPECT_EQ(customFile.readAll(), custom);
    ASSERT_TRUE(staleFile.open(QIODevice::ReadOnly));
    EXPECT_EQ(staleFile.readAll(), stale);
}

TEST_F(EffectSlotTest, StandardPickerPopulatesAndOpensTwoColumnDialog) {
    auto factory = std::make_shared<ChannelHandleFactory>();
    EffectsManager manager(config(), factory);
    ChannelHandleAndGroup output(factory->getOrCreateHandle("[MasterOutput]"), "[MasterOutput]");
    ChannelHandleAndGroup deck(factory->getOrCreateHandle("[Channel1]"), "[Channel1]");
    manager.registerOutputChannel(output);
    manager.registerInputChannel(output);
    manager.registerInputChannel(deck);
    manager.setup();
    WEffectChainPresetSelector selector(nullptr, &manager);
    QDomDocument doc;
    ASSERT_TRUE(doc.setContent(QStringLiteral("<EffectChainPresetSelector><EffectUnitGroup>[EffectRack1_EffectUnit1]</EffectUnitGroup></EffectChainPresetSelector>")));
    SkinContext context(config(), QString());
    selector.setup(doc.documentElement(), context);
    ASSERT_GT(selector.count(), 25);
    EXPECT_EQ(selector.itemText(1), QStringLiteral("DELAY"));
    bool opened = false;
    QTimer::singleShot(50, &selector, [&]() {
        auto* dialog = selector.findChild<QDialog*>(QStringLiteral("BeatFxPicker"));
        if (dialog) {
            opened = dialog->isVisible();
            int entries = 0;
            QSet<int> columns, rows;
            QPushButton* next = nullptr;
            for (auto* button : dialog->findChildren<QPushButton*>()) {
                if (button->property("presetIndex").isValid()) {
                    ++entries;
                    EXPECT_GE(button->height(), 50);
                    columns.insert(button->x());
                    rows.insert(button->y());
                }
                if (button->text() == QStringLiteral("Next")) {
                    next = button;
                }
            }
            EXPECT_EQ(entries, 14);
            EXPECT_EQ(columns.size(), 2);
            EXPECT_EQ(rows.size(), 7);
            const auto before = manager.getStandardEffectChain(0)->presetName();
            ASSERT_TRUE(next);
            next->click();
            EXPECT_EQ(manager.getStandardEffectChain(0)->presetName(), before);
            dialog->reject();
        }
    });
    selector.showPopup();
    EXPECT_TRUE(opened);
}
