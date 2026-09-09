#pragma once

#include "preferences/usersettings.h"

enum class LoadWhenDeckPlaying {
    Reject,
    Allow,
    AllowButStopDeck,
    AllowIfChannelClosed
};

namespace {
const ConfigKey kConfigKeyLoadWhenDeckPlaying = ConfigKey("[Controls]", "LoadWhenDeckPlaying");
const ConfigKey kConfigKeyAllowTrackLoadToPlayingDeck =
        ConfigKey("[Controls]", "AllowTrackLoadToPlayingDeck");
constexpr LoadWhenDeckPlaying kDefaultLoadWhenDeckPlaying = LoadWhenDeckPlaying::Reject;
} // namespace
