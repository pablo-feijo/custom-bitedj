#include "preferences/systemsettings.h"

#include <gtest/gtest.h>

#include <QByteArray>
#include <QStringList>
#include <QDomDocument>

#include "skin/legacy/skincontext.h"
#include "test/mixxxtest.h"
#include "track/track.h"
#include "widget/wtrackproperty.h"

namespace {

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
