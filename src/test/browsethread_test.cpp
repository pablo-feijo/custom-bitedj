#include <gtest/gtest.h>

#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QThread>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "library/browse/browsethread.h"
#include "library/browse/browsetablemodel.h"
#include "test/mixxxtest.h"
#include "test/soundsourceproviderregistration.h"
#include "track/globaltrackcache.h"
#include "track/track.h"

class BrowseThreadTest : public MixxxTest,
                         private SoundSourceProviderRegistration,
                         public GlobalTrackCacheSaver {
  public:
    BrowseThreadTest() {
        GlobalTrackCache::createInstance(this, [](Track* track) { delete track; });
    }
    ~BrowseThreadTest() override {
        GlobalTrackCache::destroyInstance();
    }
    void saveEvictedTrack(Track* track) noexcept override {
        EXPECT_NE(track, nullptr);
    }
};

TEST_F(BrowseThreadTest, LargeFolderIsBatchedWithoutPerTrackDelay) {
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    // Valid mono 8 kHz PCM WAV, two silent frames; entirely synthetic.
    const auto wav = QByteArray::fromHex(
            "524946462800000057415645666d74201000000001000100401f0000803e000002001000"
            "646174610400000000000000");
    for (int i = 0; i < 1000; ++i) {
        QFile file(directory.filePath(QString::number(i) + ".wav"));
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        ASSERT_EQ(file.write(wav), wav.size());
    }
    qRegisterMetaType<BrowseRowBatchPointer>();
    qRegisterMetaType<BrowseTableModel*>("BrowseTableModel*");
    auto worker = BrowseThread::getInstanceRef();
    QObject receiver;
    int rows = 0;
    int batches = 0;
    quint64 wanted = 0;
    QObject::connect(worker.data(), &BrowseThread::rowsAppended, &receiver,
            [&](BrowseRowBatchPointer batch, BrowseTableModel*, quint64 generation) {
                EXPECT_EQ(generation, wanted);
                EXPECT_LE(batch->rows.size(), 100);
                rows += batch->rows.size();
                ++batches;
            }, Qt::QueuedConnection);
    QElapsedTimer timer;
    timer.start();
    wanted = worker->executePopulation({}, nullptr, {}, directory.path());
    while (rows < 1000 && timer.elapsed() < 10000) {
        application()->processEvents();
        QThread::msleep(1);
    }
    EXPECT_EQ(rows, 1000);
    EXPECT_EQ(batches, 10);
    // Previous code slept 20 seconds for this fixture alone.
    EXPECT_LT(timer.elapsed(), 10000);
}

TEST_F(BrowseThreadTest, EmptyRequestsAreNotLostAndHaveDistinctGenerations) {
    auto worker = BrowseThread::getInstanceRef();
    QObject receiver;
    quint64 cleared = 0;
    QObject::connect(worker.data(), &BrowseThread::clearModel, &receiver,
            [&](BrowseTableModel*, quint64 generation) { cleared = generation; },
            Qt::QueuedConnection);
    const auto first = worker->executePopulation({}, nullptr);
    const auto last = worker->executePopulation({}, nullptr);
    EXPECT_GT(last, first);
    QElapsedTimer timer;
    timer.start();
    while (cleared != last && timer.elapsed() < 2000) {
        application()->processEvents();
        QThread::msleep(1);
    }
    EXPECT_EQ(cleared, last);
}

TEST_F(BrowseThreadTest, SavedMetadataCanBeReadWhileAnotherConnectionIsWriting) {
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString trackPath = directory.filePath("saved.wav");
    QFile audio(trackPath);
    ASSERT_TRUE(audio.open(QIODevice::WriteOnly));
    audio.write(QByteArray::fromHex(
            "524946462800000057415645666d74201000000001000100401f0000803e000002001000"
            "646174610400000000000000"));
    audio.close();
    const QString connection = QStringLiteral("browse-test-writer");
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
        db.setDatabaseName(directory.filePath("metadata.sqlite"));
        ASSERT_TRUE(db.open());
        QSqlQuery query(db);
        ASSERT_TRUE(query.exec("PRAGMA journal_mode=WAL"));
        ASSERT_TRUE(query.exec("CREATE TABLE track_locations(id INTEGER, location TEXT, directory TEXT)"));
        ASSERT_TRUE(query.exec("CREATE TABLE library(location INTEGER, bpm REAL, key TEXT)"));
        query.prepare("INSERT INTO track_locations VALUES(1,?,?)");
        query.addBindValue(trackPath);
        query.addBindValue(directory.path());
        ASSERT_TRUE(query.exec());
        ASSERT_TRUE(query.exec("INSERT INTO library VALUES(1,129,'5A')"));
        ASSERT_TRUE(query.exec("BEGIN IMMEDIATE"));
        auto worker = BrowseThread::getInstanceRef();
        QObject receiver;
        bool received = false;
        QObject::connect(worker.data(), &BrowseThread::rowsAppended, &receiver,
                [&](BrowseRowBatchPointer batch, BrowseTableModel*, quint64) {
                    ASSERT_EQ(batch->rows.size(), 1);
                    EXPECT_DOUBLE_EQ(batch->rows[0][COLUMN_BPM]->data(Qt::UserRole).toDouble(), 129);
                    EXPECT_EQ(batch->rows[0][COLUMN_KEY]->text(), "5A");
                    received = true;
                }, Qt::QueuedConnection);
        worker->executePopulation(mixxx::FileAccess(mixxx::FileInfo(directory.path())),
                nullptr, db.databaseName());
        QElapsedTimer timer;
        timer.start();
        while (!received && timer.elapsed() < 2000) {
            application()->processEvents();
            QThread::msleep(1);
        }
        EXPECT_TRUE(received);
        ASSERT_TRUE(query.exec("ROLLBACK"));
    }
    QSqlDatabase::removeDatabase(connection);
}

TEST_F(BrowseThreadTest, SlowConsumerBoundsQueuedRowsAndDoesNotDeadlockShutdown) {
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const auto wav = QByteArray::fromHex(
            "524946462800000057415645666d74201000000001000100401f0000803e000002001000"
            "646174610400000000000000");
    for (int i = 0; i < 600; ++i) {
        QFile file(directory.filePath(QString::number(i) + ".wav"));
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        ASSERT_EQ(file.write(wav), wav.size());
    }
    qRegisterMetaType<BrowseRowBatchPointer>();
    qRegisterMetaType<BrowseTableModel*>("BrowseTableModel*");
    auto worker = BrowseThread::getInstanceRef();
    QObject receiver;
    std::atomic<int> emittedRows{0};
    QObject::connect(worker.data(), &BrowseThread::rowsAppended, &receiver,
            [&](BrowseRowBatchPointer batch, BrowseTableModel*, quint64) {
                emittedRows += batch->rows.size();
            }, Qt::DirectConnection);
    // Leave the queued consumer stalled, like a busy UI. These events own the
    // batches until delivered or discarded with the receiver.
    QObject::connect(worker.data(), &BrowseThread::rowsAppended, &receiver,
            [](BrowseRowBatchPointer, BrowseTableModel*, quint64) {}, Qt::QueuedConnection);
    worker->executePopulation({}, nullptr, {}, directory.path());
    QElapsedTimer timer;
    timer.start();
    while (emittedRows.load() < 400 && timer.elapsed() < 5000) {
        QThread::msleep(1);
    }
    QThread::msleep(50);
    EXPECT_EQ(emittedRows.load(), 400);
    timer.restart();
    worker.reset();
    EXPECT_LT(timer.elapsed(), 1000);
}
