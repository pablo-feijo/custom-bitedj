#include "preferences/systemsettings.h"
#include "util/removablemounts.h"

#include <gtest/gtest.h>

#include <QByteArray>
#include <QStringList>
#include <QDomDocument>

#include "skin/legacy/skincontext.h"
#include "test/mixxxtest.h"
#include "track/track.h"
#include "widget/wtrackproperty.h"

namespace {

TEST(SystemSettingsTest, MountDiscoveryDoesNotProbeMediaAndDecodesPaths) {
    const QByteArray info =
            "20 1 8:1 / / rw - ext4 /dev/root rw\n"
            "21 20 8:2 / /media/pi/My\\040HDD rw shared:1 - vfat /dev/sda1 rw\n"
            "22 20 8:3 / /mnt/music rw - exfat /dev/sdb1 rw\n"
            "23 20 8:4 / /media-other/no rw - ext4 /dev/sdc1 rw\n"
            "24 20 0:9 / /mnt rw - tmpfs tmpfs rw\n"
            "malformed\n";
    const auto mounts = mixxx::parseRemovableMounts(info, SystemSettings::removableRoots());
    ASSERT_EQ(mounts.size(), 2);
    EXPECT_EQ(mounts[0].mountPoint, "/media/pi/My HDD");
    EXPECT_EQ(mounts[0].device, "/dev/sda1");
    EXPECT_EQ(mounts[1].mountPoint, "/mnt/music");
    EXPECT_EQ(mixxx::decodeMountField("literal\\134040"), "literal\\040");
}

TEST(SystemSettingsTest, MountDiscoveryUsesTopmostMountAtSamePath) {
    const auto mounts = mixxx::parseRemovableMounts(
            "1 0 8:1 / /media/USB rw - vfat /dev/sda1 rw\n"
            "2 0 8:2 / /media/USB rw - vfat /dev/sdb1 rw\n",
            SystemSettings::removableRoots());
    ASSERT_EQ(mounts.size(), 1);
    EXPECT_EQ(mounts[0].device, "/dev/sdb1");
}

class TrackSourceWidgetTest : public MixxxTest {};

TEST_F(TrackSourceWidgetTest, SourceLoadReplacementAndUnloadDoNotKeepOldTrack) {
    WTrackProperty label(nullptr, config(), nullptr, "[Channel1]", true);
    SkinContext context(config(), "test");
    QDomDocument xml;
    ASSERT_TRUE(xml.setContent(QStringLiteral("<TrackProperty><Property>source</Property></TrackProperty>")));
    label.setup(xml.documentElement(), context);
    const auto first = Track::newTemporary();
    label.slotTrackLoaded(first);
    // No SystemSettings singleton in this test: unavailable is explicit.
    EXPECT_EQ(label.text(), "N/A");
    const auto second = Track::newTemporary();
    label.slotLoadingTrack(second, first);
    EXPECT_TRUE(label.text().isEmpty());
    label.slotTrackLoaded(second);
    EXPECT_EQ(label.text(), "N/A");
    label.slotLoadingTrack({}, second);
    EXPECT_TRUE(label.text().isEmpty());
}

TEST(SystemSettingsTest, TrackSourceUsesLongestMountAndPathBoundaries) {
    const QStringList mounts{"/media/USB", "/media/USB/partition"};
    const QStringList labels{"USB1", "USB2"};
    EXPECT_EQ(SystemSettings::classifyTrackSource("/media/USB/song.wav", mounts, labels), "USB1");
    EXPECT_EQ(SystemSettings::classifyTrackSource("/media/USB/partition/song.wav", mounts, labels), "USB2");
    EXPECT_EQ(SystemSettings::classifyTrackSource("/media/USB-other/song.wav", mounts, labels), "OFFLINE");
    EXPECT_EQ(SystemSettings::classifyTrackSource("/music/song.wav", mounts, labels), "LOCAL");
    EXPECT_TRUE(SystemSettings::classifyTrackSource("", mounts, labels).isEmpty());
    EXPECT_EQ(SystemSettings::classifyTrackSource("/media/USB/song.wav", {}, {}), "OFFLINE");
}

TEST(SystemSettingsTest, RecordingIndicatorsFollowDestinationAndLoadedDecks) {
    const QStringList mounts{"/media/pi/PEN 1", "/media/pi/PEN 2"};
    const std::array<QString, 2> decks{
            "/media/pi/PEN 1/song.wav", "/media/pi/PEN 2/song.wav"};
    const auto first = SystemSettings::recordingIndicators(true,
            "/media/pi/PEN 1/Recordings/take.wav", decks, mounts);
    EXPECT_EQ(first, (std::array<bool, 3>{true, false, false}));
    const auto second = SystemSettings::recordingIndicators(true,
            "/media/pi/PEN 2/Recordings/take.wav", decks, mounts);
    EXPECT_EQ(second, (std::array<bool, 3>{false, true, false}));
    EXPECT_EQ(SystemSettings::recordingIndicators(false,
                      "/media/pi/PEN 2/Recordings/take.wav", decks, mounts),
            (std::array<bool, 3>{false, false, false}));
    EXPECT_EQ(SystemSettings::recordingIndicators(true,
                      "/media/pi/PEN 2/Recordings/take.wav",
                      {decks[0], decks[0]}, mounts),
            (std::array<bool, 3>{false, false, true}));
    EXPECT_EQ(SystemSettings::recordingIndicators(true,
                      "/media/pi/PEN 2/Recordings/take.wav",
                      {decks[1], decks[1]}, mounts),
            (std::array<bool, 3>{true, true, false}));
    EXPECT_EQ(SystemSettings::recordingIndicators(true,
                      "/media/pi/PEN 2/Recordings/take.wav", {"", ""}, mounts),
            (std::array<bool, 3>{false, false, true}));
}

TEST(SystemSettingsTest, RecordingIndicatorsDoNotMatchPrefixesOrNestedMounts) {
    const QStringList mounts{"/media/USB", "/media/USB/nested"};
    const std::array<QString, 2> decks{
            "/media/USB-other/song.wav", "/media/USB/nested/song.wav"};
    EXPECT_EQ(SystemSettings::recordingIndicators(true,
                      "/media/USB/Recordings/take.wav", decks, mounts),
            (std::array<bool, 3>{false, false, true}));
    EXPECT_EQ(SystemSettings::recordingIndicators(true,
                      "/media/USB/nested/Recordings/take.wav", decks, mounts),
            (std::array<bool, 3>{false, true, false}));
    EXPECT_EQ(SystemSettings::recordingIndicators(true,
                      "/home/pi/Recordings/take.wav", decks, mounts),
            (std::array<bool, 3>{false, false, true}));
    EXPECT_EQ(SystemSettings::recordingIndicators(true,
                      "/media/USB/nested/Recordings/take.wav", decks, {}),
            (std::array<bool, 3>{false, false, true}));
}

class ScopedUserEnvironment {
  public:
    explicit ScopedUserEnvironment(const QByteArray& user)
            : m_wasSet(qEnvironmentVariableIsSet("USER")),
              m_previous(qgetenv("USER")) {
        qputenv("USER", user);
    }

    ~ScopedUserEnvironment() {
        if (m_wasSet) {
            qputenv("USER", m_previous);
        } else {
            qunsetenv("USER");
        }
    }

  private:
    bool m_wasSet;
    QByteArray m_previous;
};

TEST(SystemSettingsTest, RemovableRootsIncludeCurrentUserMountDirectories) {
    const ScopedUserEnvironment user("bitedj-test-user");

    const QStringList roots = SystemSettings::removableRoots();

    EXPECT_TRUE(roots.contains(QStringLiteral("/media/bitedj-test-user")));
    EXPECT_TRUE(roots.contains(QStringLiteral("/run/media/bitedj-test-user")));
}

TEST(SystemSettingsTest, RemovableRootsDoNotDuplicateBasePathsWithoutUser) {
    const ScopedUserEnvironment user("");

    const QStringList roots = SystemSettings::removableRoots();

    EXPECT_EQ(1, roots.count(QStringLiteral("/media")));
    EXPECT_EQ(1, roots.count(QStringLiteral("/run/media")));
    EXPECT_FALSE(roots.contains(QStringLiteral("/media/")));
    EXPECT_FALSE(roots.contains(QStringLiteral("/run/media/")));
}

} // namespace
