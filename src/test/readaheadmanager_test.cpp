#include <gtest/gtest.h>

#include <QtDebug>
#include <QScopedPointer>

#include "engine/cachingreader/cachingreader.h"
#include "control/controlobject.h"
#include "engine/controls/loopingcontrol.h"
#include "engine/readaheadmanager.h"
#include "test/mixxxtest.h"
#include "util/assert.h"
#include "util/defs.h"
#include "util/sample.h"

namespace {
const QString kGroup = "[test]";
} // namespace

class StubReader : public CachingReader {
  public:
    StubReader()
            : CachingReader(kGroup, UserSettingsPointer()) {
    }

    CachingReader::ReadResult read(SINT startSample, SINT numSamples, bool reverse,
             CSAMPLE* buffer) override {
        Q_UNUSED(startSample);
        Q_UNUSED(reverse);
        SampleUtil::clear(buffer, numSamples);
        return CachingReader::ReadResult::AVAILABLE;
    }
};

class StubLoopControl : public LoopingControl {
  public:
    StubLoopControl()
            : LoopingControl(kGroup, UserSettingsPointer()) {
    }

    void pushTriggerReturnValue(double value) {
        m_triggerReturnValues.push_back(
                mixxx::audio::FramePos::fromEngineSamplePosMaybeInvalid(value));
    }

    void pushTargetReturnValue(double value) {
        m_targetReturnValues.push_back(
                mixxx::audio::FramePos::fromEngineSamplePosMaybeInvalid(value));
    }

    mixxx::audio::FramePos nextTrigger(bool reverse,
            mixxx::audio::FramePos currentPosition,
            mixxx::audio::FramePos* pTargetPosition) override {
        Q_UNUSED(reverse);
        Q_UNUSED(currentPosition);
        Q_UNUSED(pTargetPosition);
        RELEASE_ASSERT(!m_targetReturnValues.isEmpty());
        *pTargetPosition = m_targetReturnValues.takeFirst();
        RELEASE_ASSERT(!m_triggerReturnValues.isEmpty());
        return m_triggerReturnValues.takeFirst();
    }

  protected:
    QList<mixxx::audio::FramePos> m_triggerReturnValues;
    QList<mixxx::audio::FramePos> m_targetReturnValues;
};

class ReadAheadManagerTest : public MixxxTest {
  public:
    ReadAheadManagerTest()
            : m_beatClosestCO(ConfigKey(kGroup, "beat_closest")),
              m_beatNextCO(ConfigKey(kGroup, "beat_next")),
              m_beatPrevCO(ConfigKey(kGroup, "beat_prev")),
              m_playCO(ConfigKey(kGroup, "play")),
              m_quantizeCO(ConfigKey(kGroup, "quantize")),
              m_repeatCO(ConfigKey(kGroup, "repeat")),
              m_slipEnabledCO(ConfigKey(kGroup, "slip_enabled")),
              m_trackSamplesCO(ConfigKey(kGroup, "track_samples")),
              m_pBuffer(SampleUtil::alloc(MAX_BUFFER_LEN)) {
    }

  protected:
    void SetUp() override {
        SampleUtil::clear(m_pBuffer, MAX_BUFFER_LEN);
        m_pReader.reset(new StubReader());
        m_pLoopControl.reset(new StubLoopControl());
        m_pReadAheadManager.reset(new ReadAheadManager(m_pReader.data(),
                                                       m_pLoopControl.data()));
    }

    QScopedPointer<StubReader> m_pReader;
    QScopedPointer<StubLoopControl> m_pLoopControl;
    QScopedPointer<ReadAheadManager> m_pReadAheadManager;
    ControlObject m_beatClosestCO;
    ControlObject m_beatNextCO;
    ControlObject m_beatPrevCO;
    ControlObject m_playCO;
    ControlObject m_quantizeCO;
    ControlObject m_repeatCO;
    ControlObject m_slipEnabledCO;
    ControlObject m_trackSamplesCO;
    CSAMPLE* m_pBuffer;
};

TEST_F(ReadAheadManagerTest, PrefetchCoversMultiSecondStorageStallInBothDirections) {
    constexpr SINT position = 20 * 44100;
    for (double rate : {1.0, -1.0}) {
        m_pReadAheadManager->notifySeek(position * mixxx::kEngineChannelCount);
        HintVector hints;
        m_pReadAheadManager->hintReader(rate, &hints);
        ASSERT_EQ(hints.size(), 2);
        const auto& urgent = hints[0];
        const auto& reserve = hints[1];
        EXPECT_EQ(urgent.type, Hint::Type::CurrentPosition);
        EXPECT_EQ(urgent.frameCount, 2 * CachingReaderChunk::kFrames);
        EXPECT_GE(reserve.frameCount, 5 * 44100);
        EXPECT_LE(reserve.frameCount, 40 * CachingReaderChunk::kFrames);
        if (rate > 0) {
            EXPECT_EQ(urgent.frame, position);
            EXPECT_EQ(reserve.frame, position);
        } else {
            EXPECT_EQ(urgent.frame + urgent.frameCount, position);
            EXPECT_EQ(reserve.frame + reserve.frameCount, position);
            EXPECT_GT(urgent.frame, reserve.frame);
        }
    }
}

TEST_F(ReadAheadManagerTest, FractionalFrameLoop) {
    // If we are in reverse, a loop is enabled, and the current playposition
    // is before of the loop, we should seek to the out point of the loop.
    m_pReadAheadManager->notifySeek(0.5);
    // Trigger value means, the sample that triggers the loop (loop in)
    m_pLoopControl->pushTriggerReturnValue(20.2);
    m_pLoopControl->pushTriggerReturnValue(20.2);
    m_pLoopControl->pushTriggerReturnValue(20.2);
    m_pLoopControl->pushTriggerReturnValue(20.2);
    m_pLoopControl->pushTriggerReturnValue(20.2);
    m_pLoopControl->pushTriggerReturnValue(20.2);
    // Process value is the sample we should seek to.
    m_pLoopControl->pushTargetReturnValue(3.3);
    m_pLoopControl->pushTargetReturnValue(3.3);
    m_pLoopControl->pushTargetReturnValue(3.3);
    m_pLoopControl->pushTargetReturnValue(3.3);
    m_pLoopControl->pushTargetReturnValue(3.3);
    m_pLoopControl->pushTargetReturnValue(kNoTrigger);
    // read from start to loop trigger, overshoot 0.3
    EXPECT_EQ(20, m_pReadAheadManager->getNextSamples(1.0, m_pBuffer, 100));
    // read loop
    EXPECT_EQ(18, m_pReadAheadManager->getNextSamples(1.0, m_pBuffer, 80));
    // read loop
    EXPECT_EQ(16, m_pReadAheadManager->getNextSamples(1.0, m_pBuffer, 62));
    // read loop
    EXPECT_EQ(18, m_pReadAheadManager->getNextSamples(1.0, m_pBuffer, 46));
    // read loop
    EXPECT_EQ(16, m_pReadAheadManager->getNextSamples(1.0, m_pBuffer, 28));
    // read loop
    EXPECT_EQ(12, m_pReadAheadManager->getNextSamples(1.0, m_pBuffer, 12));

    // start 0.5 to 20.2 = 19.7
    // loop 3.3 to 20.2 = 16.9
    // 100 - 19,7 - 4 * 16,9 = 12,7
    // 12.7 + 3.3 = 16

    // The rounding error must not exceed a half frame (one samples in stereo)
    EXPECT_NEAR(16, m_pReadAheadManager->getPlaypos(), 1);
}
