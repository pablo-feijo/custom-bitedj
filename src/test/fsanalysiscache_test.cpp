#include "library/dao/fsanalysiscache.h"

#include <gtest/gtest.h>
#include <QDir>
#include <QFile>
#include <QStorageInfo>
#include "test/mixxxtest.h"

namespace {
const QString kMount = QStringLiteral("/mnt/usbtest");
class FsAnalysisCacheTest : public MixxxTest {
  protected:
    void SetUp() override {
        MixxxTest::SetUp();
        if (QStorageInfo(kMount).rootPath() != kMount) {
            GTEST_SKIP() << "Needs the private removable filesystem fixture";
        }
        FsAnalysisCache::closeFilesystemConnections(kMount);
        ASSERT_TRUE(FsAnalysisCache::clearFilesystemCache(kMount));
        QDir(kMount + "/.bitedj").rmdir(".");
        QFile audio(kMount + "/test.wav");
        ASSERT_TRUE(audio.open(QIODevice::WriteOnly));
    }
};
}

TEST_F(FsAnalysisCacheTest, PreviewMissDoesNotCreateCache) {
    FsAnalysisCache cache(config(), FsAnalysisCache::AccessMode::ReadOnly);
    EXPECT_TRUE(cache.getAnalysesForTrack(kMount + "/test.wav").isEmpty());
    EXPECT_FALSE(QFile::exists(kMount + "/.bitedj/analysis.sqlite"));
}

TEST_F(FsAnalysisCacheTest, PreviewDoesNotRepairCorruptCache) {
    ASSERT_TRUE(QDir().mkpath(kMount + "/.bitedj"));
    const auto path = kMount + "/.bitedj/analysis.sqlite";
    const QByteArray original("intentionally corrupt cache");
    { QFile file(path); ASSERT_TRUE(file.open(QIODevice::WriteOnly)); file.write(original); }
    { FsAnalysisCache cache(config(), FsAnalysisCache::AccessMode::ReadOnly);
      EXPECT_TRUE(cache.getAnalysesForTrack(kMount + "/test.wav").isEmpty()); }
    QFile file(path); ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(file.readAll(), original);
}

TEST_F(FsAnalysisCacheTest, PreviewReadsExistingCacheWithoutChangingIt) {
    auto detail = WaveformPointer::create(44100, 64, 44100, -1);
    auto summary = WaveformPointer::create(44100, 64, 44100, -1);
    for (const auto& waveform : {detail, summary}) {
        for (int i = 0; i < waveform->getDataSize(); ++i) waveform->data()[i].m_i = 0;
        waveform->setCompletion(waveform->getDataSize());
        waveform->setVersion("test-export");
        waveform->setSaveState(Waveform::SaveState::SavePending);
    }
    { FsAnalysisCache writer(config());
      ASSERT_TRUE(writer.saveTrackAnalyses(kMount + "/test.wav", detail, summary)); }
    const auto path = kMount + "/.bitedj/analysis.sqlite";
    QFile before(path); ASSERT_TRUE(before.open(QIODevice::ReadOnly));
    const auto bytes = before.readAll(); before.close();
    { FsAnalysisCache preview(config(), FsAnalysisCache::AccessMode::ReadOnly);
      const auto rows = preview.getAnalysesForTrack(kMount + "/test.wav");
      ASSERT_EQ(rows.size(), 2);
      EXPECT_EQ(rows[0].version, QString("test-export"));
      EXPECT_FALSE(rows[0].data.isEmpty());
      EXPECT_FALSE(rows[1].data.isEmpty()); }
    QFile after(path); ASSERT_TRUE(after.open(QIODevice::ReadOnly));
    EXPECT_EQ(after.readAll(), bytes);
}
