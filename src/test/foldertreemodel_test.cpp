// Regression test for the Bite DJ sidebar leaf test on browse folders:
// FolderTreeModel::directoryHasChildren() must agree with the child rows the
// sidebar actually renders (BrowseFeature::getChildDirectoryItems uses
// QDir::Dirs without QDir::Hidden). A folder whose only subdirectories are
// hidden used to report "has children", so the jog-wheel/tap leaf collapse
// never fired on it even though it rendered no child rows.
#include <gtest/gtest.h>

#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include "library/treeitem.h"

#include "library/browse/foldertreemodel.h"
#include "test/mixxxtest.h"

class FolderTreeModelTest : public MixxxTest {
  protected:
    FolderTreeModel m_model;
};

TEST_F(FolderTreeModelTest, HiddenOnlySubdirsAreNotChildren) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());
    QDir dir(tempDir.path());
    ASSERT_TRUE(dir.mkpath(".hiddendir"));
    ASSERT_TRUE(QFile(dir.filePath("track.mp3")).open(QIODevice::WriteOnly));

    EXPECT_FALSE(m_model.directoryHasChildren(tempDir.path()));
}

TEST_F(FolderTreeModelTest, VisibleSubdirIsAChild) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());
    QDir dir(tempDir.path());
    ASSERT_TRUE(dir.mkpath("Visible"));
    ASSERT_TRUE(dir.mkpath(".hiddendir"));

    EXPECT_TRUE(m_model.directoryHasChildren(tempDir.path()));
}

TEST_F(FolderTreeModelTest, FilesOnlyFolderIsALeaf) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());
    QDir dir(tempDir.path());
    ASSERT_TRUE(QFile(dir.filePath("track.mp3")).open(QIODevice::WriteOnly));

    EXPECT_FALSE(m_model.directoryHasChildren(tempDir.path()));
}

TEST_F(FolderTreeModelTest, ChildProbeUpdatesAsynchronouslyWithoutStaleIndexAccess) {
    QTemporaryDir dir;
    auto root = std::make_unique<TreeItem>();
    root->appendChild("Empty", dir.path());
    m_model.setRootItem(std::move(root));
    const auto index = m_model.index(0, 0);
    EXPECT_TRUE(m_model.hasChildren(index));
    QElapsedTimer timer;
    timer.start();
    while (m_model.hasChildren(index) && timer.elapsed() < 2000) {
        application()->processEvents();
        QThread::msleep(1);
    }
    EXPECT_FALSE(m_model.hasChildren(index));
    m_model.removeChildDirsFromCache({dir.path()});
    EXPECT_TRUE(m_model.hasChildren(index));
    m_model.removeRows(0, 1);
    // A pending result must not dereference the now-deleted item.
    for (int i = 0; i < 20; ++i) {
        application()->processEvents();
        QThread::msleep(1);
    }
    EXPECT_EQ(m_model.rowCount(), 0);
}
