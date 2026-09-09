#include "test/mixxxtest.h"
#include "mixer/deckloadpolicy.h"
#include <limits>

class DeckLoadPolicyTest : public MixxxTest {};
TEST_F(DeckLoadPolicyTest, DefaultMigrationAndCorruptionFailClosed) {
    EXPECT_EQ(mixxx::deckload::policy(config()), LoadWhenDeckPlaying::Reject);
    config()->setValue(kConfigKeyAllowTrackLoadToPlayingDeck, true);
    EXPECT_EQ(mixxx::deckload::policy(config()), LoadWhenDeckPlaying::Allow);
    for (const char* invalid : {"4", "-1", "nan", "1.5", "garbage"}) {
        config()->setValue(kConfigKeyLoadWhenDeckPlaying, QString::fromLatin1(invalid));
        EXPECT_EQ(mixxx::deckload::policy(config()), LoadWhenDeckPlaying::Reject);
    }
}
TEST_F(DeckLoadPolicyTest, RoutingNotAudioLevelDeterminesFaderProtection) {
    const QString deck="[Channel1]";
    EXPECT_FALSE(mixxx::deckload::allowed(deck, config()));
    ControlObject play(ConfigKey(deck,"play"));
    ControlObject volume(ConfigKey(deck,"volume"));
    ControlObject main(ConfigKey(deck,"main_mix"));
    play.set(1); volume.set(1); main.set(1);
    EXPECT_FALSE(mixxx::deckload::allowed(deck, config()));
    config()->setValue(kConfigKeyLoadWhenDeckPlaying, 3);
    EXPECT_FALSE(mixxx::deckload::allowed(deck, config()));
    volume.set(0);
    EXPECT_TRUE(mixxx::deckload::allowed(deck, config()));
    volume.set(1); main.set(0);
    EXPECT_TRUE(mixxx::deckload::allowed(deck, config()));
    main.set(1);
    for (double bad : {-1., std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::quiet_NaN()}) {
        volume.set(bad);
        EXPECT_FALSE(mixxx::deckload::allowed(deck, config()));
    }
    volume.set(1);
    for (int policy : {1,2}) {
        config()->setValue(kConfigKeyLoadWhenDeckPlaying, policy);
        EXPECT_TRUE(mixxx::deckload::allowed(deck, config()));
    }
    config()->setValue(kConfigKeyLoadWhenDeckPlaying, 0);
    play.set(0);
    EXPECT_TRUE(mixxx::deckload::allowed(deck, config()));
    EXPECT_TRUE(mixxx::deckload::allowed("[PreviewDeck1]", config()));
}
