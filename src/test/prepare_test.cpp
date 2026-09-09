#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <QTest>
#include "library/trackset/preparefeature.h"
#include "test/mixxxtest.h"
#include "track/track.h"

class PrepareTest : public MixxxTest {};
TEST_F(PrepareTest, UniqueOrderedQueuePersistsAndRemovalDoesNotDeleteFiles) {
    QTemporaryDir dir;
    const auto path = dir.filePath("prepare.json");
    auto first = Track::newTemporary(mixxx::FileAccess(mixxx::FileInfo(getTestDir().filePath("sine-30.wav"))));
    auto second = Track::newTemporary(mixxx::FileAccess(mixxx::FileInfo(dir.filePath("missing.wav"))));
    first->setTitle("First"); second->setTitle("Second");
    {
        PrepareModel model(nullptr, path);
        model.add({first, first, second});
        EXPECT_EQ(model.rowCount(), 2);
        QTRY_VERIFY_WITH_TIMEOUT(model.isReady(), 3000);
        ASSERT_EQ(model.rowCount(), 2);
        model.moveSelection({model.index(1, 0)}, -1);
        EXPECT_EQ(model.getTrack(model.index(0, 0)), second);
        model.search("First"); EXPECT_EQ(model.rowCount(), 1);
        model.search(""); EXPECT_EQ(model.rowCount(), 2);
    }
    {
        PrepareModel restored(nullptr, path);
        QTRY_COMPARE_WITH_TIMEOUT(restored.rowCount(), 2, 3000);
        EXPECT_EQ(restored.getTrack(restored.index(0, 0))->getTitle(), "Second");
        restored.removeTracks({restored.index(0, 0)});
        EXPECT_EQ(restored.rowCount(), 1);
        EXPECT_TRUE(QFileInfo::exists(first->getLocation()));
    }
    PrepareModel finalModel(nullptr, path);
    QTRY_COMPARE_WITH_TIMEOUT(finalModel.rowCount(), 1, 3000);
    EXPECT_EQ(finalModel.getTrack(finalModel.index(0, 0))->getTitle(), "First");
}

TEST_F(PrepareTest, QueuePresenceIgnoresSearchAndTracksRestorationAndRemoval) {
    QTemporaryDir dir;
    const auto path = dir.filePath("prepare.json");
    auto track = Track::newTemporary(mixxx::FileAccess(
            mixxx::FileInfo(getTestDir().filePath("sine-30.wav"))));
    track->setTitle("Queued track");
    {
        PrepareModel model(nullptr, path);
        QTRY_VERIFY_WITH_TIMEOUT(model.isReady(), 3000);
        EXPECT_FALSE(model.hasQueuedTracks());
        model.add({track});
        EXPECT_TRUE(model.hasQueuedTracks());
        model.search("no matching queued track");
        EXPECT_EQ(model.rowCount(), 0);
        EXPECT_TRUE(model.hasQueuedTracks());
    }
    {
        PrepareModel restored(nullptr, path);
        QTRY_VERIFY_WITH_TIMEOUT(restored.isReady(), 3000);
        EXPECT_TRUE(restored.hasQueuedTracks());
        restored.removeTracks({restored.index(0, 0)});
        EXPECT_FALSE(restored.hasQueuedTracks());
        restored.add({track});
        EXPECT_TRUE(restored.hasQueuedTracks());
        restored.clearQueue();
        EXPECT_FALSE(restored.hasQueuedTracks());
    }
    PrepareModel emptyRestore(nullptr, path);
    QTRY_VERIFY_WITH_TIMEOUT(emptyRestore.isReady(), 3000);
    EXPECT_FALSE(emptyRestore.hasQueuedTracks());
}
