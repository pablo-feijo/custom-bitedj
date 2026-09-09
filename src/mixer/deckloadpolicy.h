#pragma once

#include "util/fpclassify.h"

#include "control/controlobject.h"
#include "preferences/deckloadmode.h"

namespace mixxx::deckload {

// Routing state, not momentary silence: a quiet intro or a breakdown must
// never make a live channel replaceable. Missing/invalid controls fail closed.
inline bool channelClosed(const QString& group) {
    auto* volume = ControlObject::getControl(ConfigKey(group, "volume"));
    auto* mainMix = ControlObject::getControl(ConfigKey(group, "main_mix"));
    const auto level = volume ? volume->get() : -1.0;
    return (volume && util_isfinite(level) && level >= 0.0 && level <= 0.0001) ||
            (mainMix && mainMix->get() == 0.0);
}

inline LoadWhenDeckPlaying policy(UserSettingsPointer config) {
    if (config->exists(kConfigKeyLoadWhenDeckPlaying)) {
        bool ok = false;
        const int value = config->getValueString(kConfigKeyLoadWhenDeckPlaying).toInt(&ok);
        return ok && value >= 0 && value <= 3 ? static_cast<LoadWhenDeckPlaying>(value)
                                               : LoadWhenDeckPlaying::Reject;
    }
    if (config->exists(kConfigKeyAllowTrackLoadToPlayingDeck)) {
        return config->getValue(kConfigKeyAllowTrackLoadToPlayingDeck, false)
                ? LoadWhenDeckPlaying::Allow : LoadWhenDeckPlaying::Reject;
    }
    return kDefaultLoadWhenDeckPlaying;
}

inline bool allowed(const QString& group, UserSettingsPointer config) {
    if (group.startsWith("[PreviewDeck")) {
        return true;
    }
    // Sampler behavior is independent of main-deck replacement protection.
    if (!group.startsWith("[Channel")) return true;
    auto* play = ControlObject::getControl(ConfigKey(group, "play"));
    if (!play || !util_isfinite(play->get()) || play->get() < 0 || play->get() > 1) {
        return false;
    }
    if (play->get() == 0.0) {
        return true;
    }
    switch (policy(config)) {
    case LoadWhenDeckPlaying::Allow:
    case LoadWhenDeckPlaying::AllowButStopDeck:
        return true;
    case LoadWhenDeckPlaying::AllowIfChannelClosed:
        return channelClosed(group);
    default:
        return false;
    }
}
} // namespace mixxx::deckload
