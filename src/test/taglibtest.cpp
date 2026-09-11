#include <QDir>
#include <QtDebug>

#ifdef Q_OS_LINUX
#include <cstddef>
#include <fcntl.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#include "sources/metadatasourcetaglib.h"
#include "test/mixxxtest.h"


class TagLibTest : public testing::Test {
};

TEST_F(TagLibTest, MetadataImportNeverOpensForWriting) {
#if defined(Q_OS_LINUX) && defined(SYS_openat)
    const QStringList files = {
            QStringLiteral("cover-test-jpg.mp3"),
            QStringLiteral("cover-test-ffmpeg-aac.m4a"),
            QStringLiteral("cover-test.flac"),
            QStringLiteral("cover-test.ogg"),
            QStringLiteral("cover-test.opus"),
            QStringLiteral("cover-test.wv"),
            QStringLiteral("cover-test.wav"),
            QStringLiteral("cover-test.aiff"),
    };
    for (const auto& file : files) {
        const auto path = MixxxTest::getOrInitTestDir().filePath(
                QStringLiteral("id3-test-data/") + file);
        ASSERT_TRUE(QFileInfo::exists(path));
        SCOPED_TRACE(file.toStdString());
        // Denying with EACCES would miss the regression: TagLib retries a
        // failed read/write open as read-only. Catch the attempt itself.
        EXPECT_EXIT(([&]() {
            struct sock_filter filter[] = {
                    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
                    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_openat, 0, 3),
                    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, args[2])),
                    BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, O_ACCMODE, 0, 1),
                    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
                    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
            };
            struct sock_fprog program = {
                    static_cast<unsigned short>(sizeof(filter) / sizeof(filter[0])), filter};
            if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0 ||
                    prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) != 0) {
                _exit(2);
            }
            mixxx::TrackMetadata metadata;
            QImage cover;
            const auto imported = mixxx::MetadataSourceTagLib(path, QFileInfo(path).suffix())
                                          .importTrackMetadataAndCoverImage(&metadata, &cover, false);
            _exit(imported.first == mixxx::MetadataSource::ImportResult::Succeeded &&
                            !cover.isNull() ? 0 : 3);
        }()), ::testing::ExitedWithCode(0), "");
    }
#else
    GTEST_SKIP() << "Linux openat syscall filter required";
#endif
}

TEST_F(TagLibTest, WriteID3v2Tag) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    // Generate a file name for the temporary file
    const QString tmpFileName = tempDir.filePath("no_id3v1_mp3");

    // Create the temporary file by copying an existing file
    mixxxtest::copyFile(
            MixxxTest::getOrInitTestDir().filePath(QStringLiteral("id3-test-data/empty.mp3")),
            tmpFileName);

    // Verify that the file has no tags
    {
        TagLib::MPEG::File mpegFile(
                TAGLIB_FILENAME_FROM_QSTRING(tmpFileName));
        EXPECT_FALSE(mixxx::taglib::hasID3v1Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasID3v2Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasAPETag(mpegFile));
    }

    qDebug() << "Setting track title";

    // Write metadata -> only an ID3v2 tag should be added
    mixxx::TrackMetadata trackMetadata;
    trackMetadata.refTrackInfo().setTitle(QStringLiteral("title"));
    const auto exported =
            mixxx::MetadataSourceTagLib(
                    tmpFileName, "mp3")
                    .exportTrackMetadata(trackMetadata);
    ASSERT_EQ(mixxx::MetadataSource::ExportResult::Succeeded, exported.first);
    ASSERT_FALSE(exported.second.isNull());

    // Check that the file only has an ID3v2 tag after writing metadata
    {
        TagLib::MPEG::File mpegFile(
                TAGLIB_FILENAME_FROM_QSTRING(tmpFileName));
        EXPECT_FALSE(mixxx::taglib::hasID3v1Tag(mpegFile));
        EXPECT_TRUE(mixxx::taglib::hasID3v2Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasAPETag(mpegFile));
    }

    qDebug() << "Updating track title";

    // Write metadata again -> only the ID3v2 tag should be modified
    trackMetadata.refTrackInfo().setTitle(QStringLiteral("title2"));
    const auto exported2 =
            mixxx::MetadataSourceTagLib(
                    tmpFileName, "mp3")
                    .exportTrackMetadata(trackMetadata);
    ASSERT_EQ(mixxx::MetadataSource::ExportResult::Succeeded, exported.first);
    ASSERT_FALSE(exported.second.isNull());

    // Check that the file (still) only has an ID3v2 tag after writing metadata
    {
        TagLib::MPEG::File mpegFile(
                TAGLIB_FILENAME_FROM_QSTRING(tmpFileName));
        EXPECT_FALSE(mixxx::taglib::hasID3v1Tag(mpegFile));
        EXPECT_TRUE(mixxx::taglib::hasID3v2Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasAPETag(mpegFile));
    }
}

#ifndef __WINDOWS__ // Note: Following Windows links (*.lnk shortcuts) is not supported by Mixxx yet
TEST_F(TagLibTest, WriteID3v2TagViaLink) {
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid());

    // Generate a file name for the temporary file
    const QString tmpFileName = tempDir.filePath("no_id3v1_mp3");

    // Create the temporary file by copying an existing file
    mixxxtest::copyFile(
            MixxxTest::getOrInitTestDir().filePath(QStringLiteral("id3-test-data/empty.mp3")),
            tmpFileName);

    // Access the MP3 file indirectly via a symlink when reading & writing the tags
    const QString linkFileName = tempDir.filePath("no_id3v1_mp3_link");
    EXPECT_TRUE(QFile::link(tmpFileName, linkFileName));

    auto linkFileInfoBefore = QFileInfo(linkFileName);
    EXPECT_TRUE(linkFileInfoBefore.exists());
    EXPECT_TRUE(linkFileInfoBefore.isSymLink());
    EXPECT_EQ(linkFileInfoBefore.canonicalFilePath().toStdString(), tmpFileName.toStdString());

    // Verify that the file has no tags
    {
        TagLib::MPEG::File mpegFile(
                TAGLIB_FILENAME_FROM_QSTRING(linkFileName));
        EXPECT_FALSE(mixxx::taglib::hasID3v1Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasID3v2Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasAPETag(mpegFile));
    }

    qDebug() << "Setting track title";

    // Write metadata (via the symlink) -> only an ID3v2 tag should be added
    mixxx::TrackMetadata trackMetadata;
    trackMetadata.refTrackInfo().setTitle(QStringLiteral("title"));
    const auto exported =
            mixxx::MetadataSourceTagLib(
                    linkFileName, "mp3")
                    .exportTrackMetadata(trackMetadata);
    ASSERT_EQ(mixxx::MetadataSource::ExportResult::Succeeded, exported.first);
    ASSERT_FALSE(exported.second.isNull());

    // Check that the file only has an ID3v2 tag after writing metadata
    {
        TagLib::MPEG::File mpegFile(
                TAGLIB_FILENAME_FROM_QSTRING(linkFileName));
        EXPECT_FALSE(mixxx::taglib::hasID3v1Tag(mpegFile));
        EXPECT_TRUE(mixxx::taglib::hasID3v2Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasAPETag(mpegFile));
    }

    // Verify that the symlink still exists and still points to the correct file
    auto linkFileInfoAfter = QFileInfo(linkFileName);

    EXPECT_TRUE(linkFileInfoAfter.exists());
    EXPECT_EQ(linkFileInfoAfter.canonicalFilePath().toStdString(), tmpFileName.toStdString());
    EXPECT_TRUE(linkFileInfoAfter.isSymLink());
}
#endif
