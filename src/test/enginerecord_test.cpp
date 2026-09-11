// End-to-end cover for the recorder's file path: EngineRecord driven the way
// the sidechain thread drives it, writing a real file to a real filesystem.
//
// Saved samples and final headers must remain complete, without explicit
// writeback syscalls that can block behind a slow USB device's request queue.
#include "engine/sidechain/enginerecord.h"

#include <gtest/gtest.h>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <memory>
#include <algorithm>
#include <cmath>
#include <vector>

#include "control/controlobject.h"
#include "engine/engine.h"
#include "recording/defs_recording.h"
#include "recording/recordingmanager.h"
#include "notifications/notifications.h"
#include "test/signalpathtest.h"
#include "test/mixxxtest.h"
#include "util/types.h"

#ifdef Q_OS_LINUX
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstddef>
#endif

namespace {

constexpr int kSampleRate = 44100;
constexpr int kFramesPerBuffer = 1024;
// Cross the former 1 MiB writeback boundary multiple times.
constexpr int kBuffers = 768;
// WAV: 44 byte canonical header, 16 bit stereo frames.
constexpr int kWavHeaderBytes = 44;
constexpr int kBytesPerFrame = 4;

class EngineRecordTest : public MixxxTest {
  protected:
    void SetUp() override {
        // The controls EngineRecord reaches for; RecordingManager and the
        // engine own these in the running app.
        m_pRecStatus = std::make_unique<ControlObject>(
                ConfigKey(RECORDING_PREF_KEY, "status"));
        m_pSampleRate = std::make_unique<ControlObject>(
                ConfigKey(QStringLiteral("[App]"), QStringLiteral("samplerate")));
        m_pSampleRate->set(kSampleRate);

        m_recordingPath = getTestDataDir().filePath(QStringLiteral("enginerecord_test.wav"));
        QFile::remove(m_recordingPath);
        config()->set(ConfigKey(RECORDING_PREF_KEY, "Path"), ConfigValue(m_recordingPath));
        config()->set(ConfigKey(RECORDING_PREF_KEY, "Encoding"), ConfigValue(ENCODING_WAVE));
        config()->set(ConfigKey(RECORDING_PREF_KEY, "CueEnabled"), ConfigValue(0));

        m_pRecord = std::make_unique<EngineRecord>(config());
        m_buffer.assign(kFramesPerBuffer * mixxx::kEngineChannelCount, 0.25f);
    }

    void TearDown() override {
        m_pRecord.reset();
        QFile::remove(m_recordingPath);
    }

    // One turn of the sidechain's drain loop.
    void process() {
        m_pRecord->process(m_buffer.data(),
                static_cast<int>(m_buffer.size()));
    }

    std::unique_ptr<ControlObject> m_pRecStatus;
    std::unique_ptr<ControlObject> m_pSampleRate;
    std::unique_ptr<EngineRecord> m_pRecord;
    std::vector<CSAMPLE> m_buffer;
    QString m_recordingPath;
};

TEST_F(EngineRecordTest, writesACompleteFile) {
    QSignalSpy recordingSpy(m_pRecord.get(), &EngineRecord::isRecording);

    m_pRecStatus->set(RECORD_READY);
    for (int i = 0; i < kBuffers; ++i) {
        process();
    }
    ASSERT_EQ(RECORD_ON, m_pRecStatus->get()) << "recording never started";

    // The stop the DJ asks for: status goes off, the next turn of the loop
    // flushes the encoder and closes the file.
    m_pRecStatus->set(RECORD_OFF);
    process();

    ASSERT_GE(recordingSpy.count(), 2);
    // Started, then stopped, and neither carried the error flag.
    EXPECT_TRUE(recordingSpy.first().at(0).toBool());
    EXPECT_FALSE(recordingSpy.first().at(1).toBool());
    EXPECT_FALSE(recordingSpy.last().at(0).toBool());
    EXPECT_FALSE(recordingSpy.last().at(1).toBool());

    const QFileInfo recorded(m_recordingPath);
    ASSERT_TRUE(recorded.exists()) << "no file was written";
    // Every supplied buffer must be present in the finalized file.
    const qint64 expectedBytes = static_cast<qint64>(kWavHeaderBytes) +
            static_cast<qint64>(kBuffers) * kFramesPerBuffer * kBytesPerFrame;
    EXPECT_EQ(expectedBytes, recorded.size());

    QFile file(m_recordingPath);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    const QByteArray header = file.read(12);
    // Closing must patch the header to produce a valid RIFF/WAVE file.
    EXPECT_EQ(QByteArray("RIFF"), header.left(4));
    EXPECT_EQ(QByteArray("WAVE"), header.mid(8, 4));
    ASSERT_TRUE(file.seek(kWavHeaderBytes));
    const auto pcm = file.readAll();
    bool samplesMatch = true;
    for (int i = 0; i + 1 < pcm.size(); i += 2) {
        const int value = static_cast<unsigned char>(pcm[i]) |
                (int(static_cast<unsigned char>(pcm[i + 1])) << 8);
        if (std::abs(value - 8192) > 1) { samplesMatch = false; break; }
    }
    EXPECT_TRUE(samplesMatch) << "Saved PCM differs from the supplied audio";
}

TEST_F(EngineRecordTest, RecordingDoesNotForceWriteback) {
#if defined(Q_OS_LINUX) && defined(SYS_sync_file_range)
    // Install the filter only in a child: the former 1 MiB cache limiter dies
    // here even on fast test disks, where its USB queue stall is not reproducible.
    // Other tests and the parent retain their normal syscall permissions.
    EXPECT_EXIT(([&]() {
        struct sock_filter filter[] = {
                BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
                BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_sync_file_range, 0, 1),
                BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
                BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
        };
        struct sock_fprog program = {
                static_cast<unsigned short>(sizeof(filter) / sizeof(filter[0])), filter};
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0 ||
                prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) != 0) {
            _exit(2);
        }
        m_pRecStatus->set(RECORD_READY);
        for (int i = 0; i < kBuffers; ++i) {
            process();
        }
        if (m_pRecStatus->get() != RECORD_ON) {
            _exit(3);
        }
        m_pRecStatus->set(RECORD_OFF);
        process();
        const qint64 expectedBytes = kWavHeaderBytes +
                static_cast<qint64>(kBuffers) * kFramesPerBuffer * kBytesPerFrame;
        _exit(QFileInfo(m_recordingPath).size() == expectedBytes ? 0 : 4);
    }()), ::testing::ExitedWithCode(0), "");
#else
    GTEST_SKIP() << "Linux sync_file_range syscall filter required";
#endif
}

TEST_F(EngineRecordTest, splittingStartsANewFileAndKeepsTheOldOne) {
    m_pRecStatus->set(RECORD_READY);
    for (int i = 0; i < kBuffers; ++i) {
        process();
    }
    ASSERT_EQ(RECORD_ON, m_pRecStatus->get());

    // RecordingManager splits by pointing Path at the next part and asking for
    // a continue; the new file must retain every subsequent sample.
    const QString partTwoPath =
            getTestDataDir().filePath(QStringLiteral("enginerecord_test_part2.wav"));
    QFile::remove(partTwoPath);
    config()->set(ConfigKey(RECORDING_PREF_KEY, "Path"), ConfigValue(partTwoPath));
    m_pRecStatus->set(RECORD_SPLIT_CONTINUE);
    process();
    ASSERT_EQ(RECORD_ON, m_pRecStatus->get()) << "split did not resume recording";

    for (int i = 0; i < kBuffers; ++i) {
        process();
    }
    m_pRecStatus->set(RECORD_OFF);
    process();

    const qint64 expectedFirst = static_cast<qint64>(kWavHeaderBytes) +
            static_cast<qint64>(kBuffers) * kFramesPerBuffer * kBytesPerFrame;
    // The part that was split away is closed and complete...
    EXPECT_EQ(expectedFirst, QFileInfo(m_recordingPath).size());
    // ...and the second part holds everything after the split (one buffer of
    // which was the process() call that performed it).
    const qint64 expectedSecond = static_cast<qint64>(kWavHeaderBytes) +
            static_cast<qint64>(kBuffers + 1) * kFramesPerBuffer * kBytesPerFrame;
    EXPECT_EQ(expectedSecond, QFileInfo(partTwoPath).size());
    QFile::remove(partTwoPath);
}

TEST_F(EngineRecordTest, WriteFailureStopsRecordingAndNextRecordingCanStart) {
#ifdef Q_OS_LINUX
    // /dev/full accepts opens but rejects writes with ENOSPC, unlike a bad path.
    ASSERT_TRUE(QFile::link(QStringLiteral("/dev/full"), m_recordingPath));
    QSignalSpy recordingSpy(m_pRecord.get(), &EngineRecord::isRecording);
    m_pRecStatus->set(RECORD_READY);
    for (int i = 0; i < 32 && m_pRecStatus->get() != RECORD_OFF; ++i) process();
    ASSERT_EQ(RECORD_OFF, m_pRecStatus->get());
    ASSERT_FALSE(recordingSpy.isEmpty());
    EXPECT_FALSE(recordingSpy.last().at(0).toBool());
    EXPECT_TRUE(recordingSpy.last().at(1).toBool());
    ASSERT_TRUE(QFile::remove(m_recordingPath));
    recordingSpy.clear();
    m_pRecStatus->set(RECORD_READY);
    process();
    ASSERT_EQ(RECORD_ON, m_pRecStatus->get());
    m_pRecStatus->set(RECORD_OFF);
    process();
    EXPECT_FALSE(recordingSpy.last().at(1).toBool());
    EXPECT_GT(QFileInfo(m_recordingPath).size(), kWavHeaderBytes);
#else
    GTEST_SKIP() << "Requires Linux /dev/full";
#endif
}

TEST_F(EngineRecordTest, OverflowFinalizesPartialWavAndReportsFailure) {
    QSignalSpy spy(m_pRecord.get(), &EngineRecord::isRecording);
    m_pRecStatus->set(RECORD_READY);
    process();
    ASSERT_EQ(RECORD_ON, m_pRecStatus->get());
    m_pRecord->onBufferOverflow();
    process();
    EXPECT_EQ(RECORD_OFF, m_pRecStatus->get());
    ASSERT_FALSE(spy.isEmpty());
    EXPECT_TRUE(spy.last().at(1).toBool());
    QFile file(m_recordingPath); ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    const auto bytes = file.readAll();
    ASSERT_EQ(bytes.size(), kWavHeaderBytes + kFramesPerBuffer * kBytesPerFrame);
    const auto u32 = [&bytes](int offset) {
        const auto* p = reinterpret_cast<const unsigned char*>(bytes.constData()) + offset;
        return unsigned(p[0]) | (unsigned(p[1]) << 8) | (unsigned(p[2]) << 16) | (unsigned(p[3]) << 24);
    };
    EXPECT_EQ(u32(4), unsigned(bytes.size() - 8));
    EXPECT_EQ(u32(40), unsigned(bytes.size() - kWavHeaderBytes));
}

TEST_F(EngineRecordTest, OverflowDuringPendingStopReportsFailure) {
    QSignalSpy spy(m_pRecord.get(), &EngineRecord::isRecording);
    m_pRecStatus->set(RECORD_READY);
    process();
    m_pRecStatus->set(RECORD_OFF);
    m_pRecord->onBufferOverflow();
    process();
    ASSERT_FALSE(spy.isEmpty());
    EXPECT_FALSE(spy.last().at(0).toBool());
    EXPECT_TRUE(spy.last().at(1).toBool());
}

TEST_F(EngineRecordTest, OverflowDuringPendingSplitStopsRecording) {
    QSignalSpy spy(m_pRecord.get(), &EngineRecord::isRecording);
    m_pRecStatus->set(RECORD_READY);
    process();
    m_pRecStatus->set(RECORD_SPLIT_CONTINUE);
    m_pRecord->onBufferOverflow();
    process();
    EXPECT_EQ(RECORD_OFF, m_pRecStatus->get());
    ASSERT_FALSE(spy.isEmpty());
    EXPECT_FALSE(spy.last().at(0).toBool());
    EXPECT_TRUE(spy.last().at(1).toBool());
}

TEST_F(EngineRecordTest, UnavailableDestinationReportsFailure) {
    config()->set(ConfigKey(RECORDING_PREF_KEY, "Path"),
            ConfigValue(getTestDataDir().filePath("missing-recording-directory/take.wav")));
    QSignalSpy spy(m_pRecord.get(), &EngineRecord::isRecording);
    m_pRecStatus->set(RECORD_READY);
    process();
    EXPECT_EQ(RECORD_OFF, m_pRecStatus->get());
    ASSERT_FALSE(spy.isEmpty());
    EXPECT_FALSE(spy.last().at(0).toBool());
    EXPECT_TRUE(spy.last().at(1).toBool());
}

TEST_F(EngineRecordTest, FilePositionsRemainValidBeyondTwoGiB) {
    ASSERT_EQ(m_pRecord->updateFromPreferences(), 0);
    ASSERT_TRUE(m_pRecord->openFile());
    constexpr qint64 offset = qint64(3) * 1024 * 1024 * 1024;
    m_pRecord->seek(offset);
    ASSERT_EQ(m_pRecord->tell(), offset);
    const unsigned char sample = 0;
    m_pRecord->write(nullptr, &sample, 0, 1);
    EXPECT_EQ(m_pRecord->tell(), offset + 1);
    EXPECT_EQ(m_pRecord->filelen(), offset + 1);
    m_pRecord->closeFile();
}

class RecordingManagerTest : public BaseSignalPathTest {};

TEST_F(RecordingManagerTest, PreservesExistingAudioAndCueFiles) {
    config()->set(ConfigKey(RECORDING_PREF_KEY, "Directory"), getTestDataDir().path());
    config()->set(ConfigKey(RECORDING_PREF_KEY, "Encoding"), ConfigValue(ENCODING_WAVE));
    // Reserve the timestamp and first suffix on either side of the current
    // second so crossing a clock tick cannot hide a filename collision.
    const auto now = QDateTime::currentDateTime();
    QStringList reserved;
    for (int second = -60; second <= 60; ++second) {
        const auto base = getTestDataDir().filePath(
                now.addSecs(second).toString("yyyy-MM-dd_hh'h'mm'm'ss's'"));
        for (const auto& suffix : {QStringLiteral(".wav"), QStringLiteral("-1.cue")}) {
            const auto path = base + suffix;
            QFile existing(path);
            ASSERT_TRUE(existing.open(QIODevice::WriteOnly));
            ASSERT_EQ(existing.write("previous take"), 13);
            reserved.append(path);
        }
    }
    RecordingManager manager(config(), m_pEngineMixer);
    manager.startRecording();
    EXPECT_TRUE(manager.getRecordingLocation().endsWith("-2.wav"));
    for (const auto& path : reserved) {
        QFile existing(path);
        ASSERT_TRUE(existing.open(QIODevice::ReadOnly));
        EXPECT_EQ(existing.readAll(), QByteArray("previous take"));
    }
}

TEST_F(RecordingManagerTest, InterruptedStopDoesNotReportSuccessfulSave) {
    Notifications notifications;
    QSignalSpy spy(&notifications, &Notifications::messagePosted);
    RecordingManager manager(config(), m_pEngineMixer);
    manager.slotIsRecording(true, false);
    manager.stopRecording(true);
    manager.slotIsRecording(false, false);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.last().at(1).toInt(), int(Notifications::Severity::Error));
    spy.clear();
    manager.slotIsRecording(true, false);
    manager.stopRecording();
    manager.slotIsRecording(false, false);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.last().at(1).toInt(), int(Notifications::Severity::Info));
}

TEST_F(RecordingManagerTest, LowSpaceWarningUsesNotificationStrip) {
    Notifications notifications;
    QSignalSpy spy(&notifications, &Notifications::messagePosted);
    RecordingManager manager(config(), m_pEngineMixer);
    manager.slotFreeSpaceAvailable(1024);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.last().at(1).toInt(), int(Notifications::Severity::Warning));
    manager.slotFreeSpaceAvailable(1024);
    EXPECT_EQ(spy.count(), 1);
}

} // namespace
