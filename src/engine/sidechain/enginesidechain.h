#pragma once

#include <QAtomicInt>

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QList>

#include "preferences/usersettings.h"
#include "soundio/soundmanagerutil.h"
#include "util/fifo.h"
#include "util/mutex.h"
#include "util/types.h"

class SideChainWorker;

class EngineSideChain : public QThread, public AudioDestination {
    Q_OBJECT
  public:
    EngineSideChain(UserSettingsPointer pConfig, CSAMPLE* sidechainMix);
    ~EngineSideChain() override;

    // Not thread-safe, wait-free. Submit buffer of samples to the sidechain for
    // processing. Should only be called from a single writer thread (typically
    // the engine callback).
    void writeSamples(const CSAMPLE* pBuffer, int iFrames);

    // Thin wrapper around writeSamples that is used by SoundManager when receiving
    // from a sound card input instead of the engine
    void receiveBuffer(const AudioInput& input,
            const CSAMPLE* pBuffer,
            unsigned int iFrames) override;

    // Thread-safe, blocking.
    void addSideChainWorker(SideChainWorker* pWorker);

    // 4 MiB: about 11.9 seconds of stereo audio at 44.1 kHz.
    static constexpr int SIDECHAIN_BUFFER_SIZE = 1048576;
    static constexpr int WORK_BUFFER_SIZE = 65536;

  private:
    void run() override;

    UserSettingsPointer m_pConfig;
    // Indicates that the thread should exit.
    volatile bool m_bStopThread;

    FIFO<CSAMPLE> m_sampleFifo;
    QAtomicInt m_bufferOverflow{0};
    CSAMPLE* m_pWorkBuffer;
    CSAMPLE* m_pSidechainMix;

    // Provides thread safety around the wait condition below.
    QMutex m_waitLock;
    // Allows sleeping until we have samples to process.
    QWaitCondition m_waitForSamples;

    // Sidechain workers registered with EngineSideChain.
    MMutex m_workerLock;
    QList<SideChainWorker*> m_workers GUARDED_BY(m_workerLock);
};
