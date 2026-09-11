#include "library/dao/fscueoverridestore.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QGlobalStatic>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QWaitCondition>
#include <algorithm>
#include <deque>
#include <functional>
#include <optional>

#include "library/dao/fsstore.h"
#include "preferences/systemsettings.h"
#include "track/cue.h"
#include "track/track.h"
#include "util/assert.h"
#include "util/rtscheduling.h"

namespace {

// Per-filesystem layout, alongside the analysis cache in the same .bitedj dir.
const QString kStoreDbName = QStringLiteral("cues.sqlite");
const char kLogTag[] = "FsCueOverrideStore";

// No Track or SQLite connection crosses threads: jobs own serialized cue data
// and open their own short-lived connection. Low I/O priority matters as much
// as CPU priority when a recording and a playing deck share this USB drive.
class CueWriteThread final : public QThread {
  public:
    CueWriteThread() {
        setObjectName(QStringLiteral("CueWriteThread"));
        start(QThread::LowPriority);
    }
    ~CueWriteThread() override {
        {
            QMutexLocker locker(&m_mutex);
            m_stopping = true;
            m_ready.wakeAll();
        }
        wait();
    }
    void enqueue(std::function<void()> job) {
        QMutexLocker locker(&m_mutex);
        m_jobs.push_back(std::move(job));
        m_ready.wakeAll();
    }
    bool drain(int timeoutMillis) {
        QElapsedTimer timer;
        timer.start();
        QMutexLocker locker(&m_mutex);
        while (m_running || !m_jobs.empty()) {
            if (timeoutMillis < 0) {
                m_done.wait(&m_mutex);
                continue;
            }
            const auto remaining = timeoutMillis - timer.elapsed();
            if (remaining <= 0) {
                return false;
            }
            m_done.wait(&m_mutex, static_cast<unsigned long>(remaining));
        }
        return true;
    }
  protected:
    void run() override {
        mixxx::demoteCurrentThreadToBackground("CueWriteThread");
        QMutexLocker locker(&m_mutex);
        for (;;) {
            while (m_jobs.empty() && !m_stopping) {
                m_ready.wait(&m_mutex);
            }
            if (m_jobs.empty()) {
                return;
            }
            auto job = std::move(m_jobs.front());
            m_jobs.pop_front();
            m_running = true;
            locker.unlock();
            job();
            locker.relock();
            m_running = false;
            m_done.wakeAll();
        }
    }
  private:
    QMutex m_mutex;
    QWaitCondition m_ready;
    QWaitCondition m_done;
    std::deque<std::function<void()>> m_jobs;
    bool m_running = false;
    bool m_stopping = false;
};

Q_GLOBAL_STATIC(CueWriteThread, cueWriteThread)

const QString kCreateTableDdl = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS cue_overrides ("
        "relpath TEXT PRIMARY KEY NOT NULL, "
        "version INTEGER NOT NULL, "
        "updated_at TEXT, "
        "cues TEXT NOT NULL)");

// Payload format version, so a future change to the stored fields can be told
// apart from the current one instead of being misread.
constexpr int kPayloadVersion = 1;

// Baseline marker for a track whose override was cleared while it stayed
// loaded. Not valid JSON, so it can never equal a serialized cue set.
const QByteArray kSuppressedBaseline = QByteArrayLiteral("\x01suppressed");

// An empty cue set. Stored as an override in its own right (the DJ deleted
// every cue), but not worth creating a first entry for.
const QByteArray kEmptyCues = QByteArrayLiteral("[]");

const QString kSlotKey = QStringLiteral("slot");
const QString kTypeKey = QStringLiteral("type");
const QString kPositionKey = QStringLiteral("pos");
const QString kEndPositionKey = QStringLiteral("end");
const QString kLabelKey = QStringLiteral("label");
const QString kColorKey = QStringLiteral("color");

/// One stored cue. `slot` is the hotcue index for both banks, or
/// Cue::kNoHotCue for the main cue. Positions are seconds from the start of
/// the file, so they survive a move to a unit that decodes differently.
struct StoredCue {
    int slot = Cue::kNoHotCue;
    int type = static_cast<int>(mixxx::CueType::HotCue);
    double startSeconds = 0.0;
    double endSeconds = -1.0;
    QString label;
    mixxx::RgbColor::code_t color = 0;
};

bool isManagedSlot(int slot) {
    return (slot >= mixxx::kHotCueBankStart &&
                   slot < mixxx::kHotCueBankStart + mixxx::kHotCueBankSize) ||
            (slot >= mixxx::kMemoryCueBankStart &&
                    slot < mixxx::kMemoryCueBankStart + mixxx::kMemoryCueBankSize);
}

/// Whether the store owns this cue, i.e. whether it is one the DJ can set from
/// the pads: both hotcue banks, plus the main cue. Everything else (intro,
/// outro, the analyzer's N60dBSound range) is left to the ordinary library.
bool isManagedCue(const CuePointer& pCue) {
    if (pCue->getType() == mixxx::CueType::MainCue) {
        return true;
    }
    if (pCue->getType() != mixxx::CueType::HotCue &&
            pCue->getType() != mixxx::CueType::Loop) {
        return false;
    }
    return isManagedSlot(pCue->getHotCue());
}

} // anonymous namespace

QMutex FsCueOverrideStore::s_baselineMutex;
QHash<QString, QByteArray> FsCueOverrideStore::s_baselines;
QHash<QString, QByteArray> FsCueOverrideStore::s_importedCues;
QHash<QString, QPair<quint64, QByteArray>> FsCueOverrideStore::s_pendingCues;
quint64 FsCueOverrideStore::s_nextWrite = 0;

QByteArray FsCueOverrideStore::serializeCues(const Track& track) {
    const mixxx::audio::SampleRate sampleRate = track.getSampleRate();
    VERIFY_OR_DEBUG_ASSERT(sampleRate.isValid()) {
        return kEmptyCues;
    }

    QList<StoredCue> storedCues;
    const QList<CuePointer> cuePoints = track.getCuePoints();
    for (const CuePointer& pCue : cuePoints) {
        if (!isManagedCue(pCue)) {
            continue;
        }
        const mixxx::audio::FramePos startPosition = pCue->getPosition();
        if (!startPosition.isValid()) {
            continue;
        }
        StoredCue storedCue;
        storedCue.slot = pCue->getType() == mixxx::CueType::MainCue
                ? Cue::kNoHotCue
                : pCue->getHotCue();
        storedCue.type = static_cast<int>(pCue->getType());
        storedCue.startSeconds = startPosition.value() / sampleRate;
        const mixxx::audio::FramePos endPosition = pCue->getEndPosition();
        if (endPosition.isValid()) {
            storedCue.endSeconds = endPosition.value() / sampleRate;
        }
        storedCue.label = pCue->getLabel();
        storedCue.color = pCue->getColor();
        storedCues.append(storedCue);
    }

    // Sorted so that the same cue set always serializes to the same bytes,
    // which is what makes the baseline comparison meaningful.
    std::sort(storedCues.begin(),
            storedCues.end(),
            [](const StoredCue& a, const StoredCue& b) { return a.slot < b.slot; });

    QJsonArray array;
    for (const StoredCue& storedCue : std::as_const(storedCues)) {
        QJsonObject object;
        object.insert(kSlotKey, storedCue.slot);
        object.insert(kTypeKey, storedCue.type);
        object.insert(kPositionKey, storedCue.startSeconds);
        if (storedCue.endSeconds >= 0.0) {
            object.insert(kEndPositionKey, storedCue.endSeconds);
        }
        if (!storedCue.label.isEmpty()) {
            object.insert(kLabelKey, storedCue.label);
        }
        object.insert(kColorKey, static_cast<int>(storedCue.color));
        array.append(object);
    }
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

void FsCueOverrideStore::applyPayload(Track* pTrack, const QByteArray& payload) {
    const mixxx::audio::SampleRate sampleRate = pTrack->getSampleRate();
    VERIFY_OR_DEBUG_ASSERT(sampleRate.isValid()) {
        // Stored positions are seconds; without a rate every cue would land
        // on frame 0.
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        qWarning() << "FsCueOverrideStore: ignoring unreadable cue override for"
                   << pTrack->getLocation() << parseError.errorString();
        return;
    }

    QHash<int, StoredCue> cuesBySlot;
    std::optional<StoredCue> mainCue;
    const QJsonArray array = document.array();
    for (const QJsonValue& value : array) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        StoredCue storedCue;
        storedCue.slot = object.value(kSlotKey).toInt(Cue::kNoHotCue);
        storedCue.type = object.value(kTypeKey).toInt(
                static_cast<int>(mixxx::CueType::HotCue));
        storedCue.startSeconds = object.value(kPositionKey).toDouble(-1.0);
        storedCue.endSeconds = object.value(kEndPositionKey).toDouble(-1.0);
        storedCue.label = object.value(kLabelKey).toString();
        storedCue.color = static_cast<mixxx::RgbColor::code_t>(
                object.value(kColorKey).toInt(0));
        if (storedCue.startSeconds < 0.0) {
            continue;
        }
        if (storedCue.type == static_cast<int>(mixxx::CueType::MainCue)) {
            mainCue = storedCue;
        } else if (isManagedSlot(storedCue.slot)) {
            cuesBySlot.insert(storedCue.slot, storedCue);
        }
    }

    const auto framePosOf = [sampleRate](double seconds) {
        return seconds < 0.0 ? mixxx::audio::kInvalidFramePos
                             : mixxx::audio::FramePos(seconds * sampleRate);
    };
    // A pad holds a plain cue or a saved loop, and which one it is follows the
    // range rather than the stored type — the same rule the rekordbox import
    // uses, and the one that keeps a hand-edited or future-version payload
    // from putting an unusable cue type on a deck.
    const auto typeOf = [](const StoredCue& storedCue) {
        return storedCue.endSeconds >= 0.0 ? mixxx::CueType::Loop
                                           : mixxx::CueType::HotCue;
    };

    // Update the slots that survive in place and drop the ones the override
    // does not have: the track is handed straight to a deck, and one that is
    // already playing must not have its pads rebuilt underneath it.
    QList<CuePointer> staleCues;
    const QList<CuePointer> cuePoints = pTrack->getCuePoints();
    for (const CuePointer& pCue : cuePoints) {
        if (pCue->getType() != mixxx::CueType::HotCue &&
                pCue->getType() != mixxx::CueType::Loop) {
            continue;
        }
        const int slot = pCue->getHotCue();
        if (!isManagedSlot(slot)) {
            continue;
        }
        const auto it = cuesBySlot.constFind(slot);
        if (it == cuesBySlot.constEnd()) {
            staleCues.append(pCue);
            continue;
        }
        pCue->setStartAndEndPosition(
                framePosOf(it->startSeconds), framePosOf(it->endSeconds));
        pCue->setType(typeOf(*it));
        pCue->setLabel(it->label);
        pCue->setColor(mixxx::RgbColor(it->color));
        cuesBySlot.erase(it);
    }
    for (const CuePointer& pCue : std::as_const(staleCues)) {
        pTrack->removeCue(pCue);
    }

    for (auto it = cuesBySlot.constBegin(); it != cuesBySlot.constEnd(); ++it) {
        const CuePointer pCue = pTrack->createAndAddCue(
                typeOf(*it),
                it->slot,
                framePosOf(it->startSeconds),
                framePosOf(it->endSeconds),
                mixxx::RgbColor(it->color));
        pCue->setLabel(it->label);
    }

    if (mainCue) {
        pTrack->setMainCuePosition(framePosOf(mainCue->startSeconds));
        const CuePointer pMainCue = pTrack->findCueByType(mixxx::CueType::MainCue);
        if (pMainCue) {
            pMainCue->setLabel(mainCue->label);
            pMainCue->setColor(mixxx::RgbColor(mainCue->color));
        }
    }
}

void FsCueOverrideStore::applyOverrides(Track* pTrack) {
    VERIFY_OR_DEBUG_ASSERT(pTrack) {
        return;
    }
    const QString location = pTrack->getLocation();
    if (location.isEmpty() || !SystemSettings::isOnRemovableMedia(location)) {
        // A track on the internal drive keeps its cues in the library, where
        // they were already; there is no drive for them to follow.
        return;
    }
    if (!pTrack->getSampleRate().isValid()) {
        // Stored positions are seconds and cannot be placed without one. Leave
        // the track unbaselined so a later load (by then the file has been
        // decoded, so the rate is known) applies the override instead.
        return;
    }

    QByteArray payload;
    bool found = false;
    QByteArray imported;
    bool hasImported = false;
    if (readOverride(location, &payload, &found) && found) {
        // What the source library exported for this track, captured before the
        // override is laid over it: the rekordbox ANLZ cues that readAnalyze
        // has just put on, or the Serato markers the library database holds.
        // Settings → Clear puts exactly this back, so clearing takes off the
        // DJ's own edits and nothing else.
        imported = serializeCues(*pTrack);
        hasImported = true;
        applyPayload(pTrack, payload);
    }

    // Baseline what the track carries now, override applied or not, so that
    // only an edit made from here on counts as a change worth storing.
    const QByteArray baseline = serializeCues(*pTrack);
    QMutexLocker locker(&s_baselineMutex);
    s_baselines.insert(location, baseline);
    if (hasImported) {
        s_importedCues.insert(location, imported);
    }
}

void FsCueOverrideStore::flushIfChanged(const Track& track) {
    const QString location = track.getLocation();
    if (location.isEmpty() || !track.getSampleRate().isValid() ||
            !SystemSettings::isOnRemovableMedia(location)) {
        return;
    }

    const QByteArray payload = serializeCues(track);
    {
        QMutexLocker locker(&s_baselineMutex);
        const auto it = s_baselines.constFind(location);
        if (it == s_baselines.constEnd()) {
            // Never seen by applyOverrides(): treat the baseline as empty, so
            // a track first cued in this session is stored, but one that has
            // no cues at all does not get an entry of its own.
            if (payload == kEmptyCues) {
                return;
            }
        } else if (*it == kSuppressedBaseline || *it == payload) {
            return;
        }
    }

    if (!writeOverride(location, payload)) {
        return;
    }
    QMutexLocker locker(&s_baselineMutex);
    s_baselines.insert(location, payload);
}

void FsCueOverrideStore::queueIfChanged(const Track& track) {
    const QString location = track.getLocation();
    if (location.isEmpty() || !track.getSampleRate().isValid() ||
            !SystemSettings::isOnRemovableMedia(location)) {
        return;
    }
    const QByteArray payload = serializeCues(track);
    QMutexLocker locker(&s_baselineMutex);
    const auto baseline = s_baselines.constFind(location);
    if (baseline != s_baselines.constEnd() && *baseline == kSuppressedBaseline) {
        return;
    }
    if (!s_pendingCues.contains(location) &&
            payload == (baseline == s_baselines.constEnd() ? kEmptyCues : *baseline)) {
        return;
    }
    const quint64 serial = ++s_nextWrite;
    s_pendingCues.insert(location, qMakePair(serial, payload));
    cueWriteThread->enqueue([location, payload, serial]() {
        if (!writeOverride(location, payload)) {
            // Retain the pending snapshot so a reload cannot silently discard
            // an edit that failed to reach the drive.
            return;
        }
        QMutexLocker locker(&s_baselineMutex);
        const auto pending = s_pendingCues.constFind(location);
        if (pending != s_pendingCues.constEnd() && pending->first == serial) {
            s_pendingCues.remove(location);
            if (s_baselines.value(location) != kSuppressedBaseline) {
                s_baselines.insert(location, payload);
            }
        }
    });
}

bool FsCueOverrideStore::flushPendingWrites(int timeoutMillis) {
    if (cueWriteThread.exists() && !cueWriteThread->drain(timeoutMillis)) {
        return false;
    }
    QMutexLocker locker(&s_baselineMutex);
    return s_pendingCues.isEmpty();
}

bool FsCueOverrideStore::readOverride(
        const QString& trackLocation, QByteArray* pPayload, bool* pFound) {
    {
        QMutexLocker locker(&s_baselineMutex);
        const auto pending = s_pendingCues.constFind(trackLocation);
        if (pending != s_pendingCues.constEnd()) {
            *pPayload = pending->second;
            *pFound = true;
            return true;
        }
    }
    *pFound = false;
    FsStoreTarget target;
    if (!FsStoreTarget::resolveForFile(trackLocation, kStoreDbName, &target)) {
        return false;
    }

    ScopedFsStore store(kLogTag);
    if (!store.open(target, QString())) {
        // No store on this drive (the common case for a track that has never
        // been cued here), or one that could not be opened.
        return true;
    }

    QSqlQuery query(store.database());
    query.prepare(QStringLiteral(
            "SELECT cues FROM cue_overrides "
            "WHERE relpath = :relpath AND version = :version"));
    query.bindValue(QStringLiteral(":relpath"), target.relPath);
    query.bindValue(QStringLiteral(":version"), kPayloadVersion);
    if (!query.exec()) {
        // Also the path taken by a store written by a future schema.
        qDebug() << "FsCueOverrideStore: cannot read overrides for" << target.relPath
                 << query.lastError().text();
        return true;
    }
    if (query.next()) {
        *pPayload = query.value(0).toString().toUtf8();
        *pFound = true;
    }
    return true;
}

bool FsCueOverrideStore::writeOverride(
        const QString& trackLocation, const QByteArray& payload) {
    FsStoreTarget target;
    if (!FsStoreTarget::resolveForFile(trackLocation, kStoreDbName, &target)) {
        return false;
    }
    if (!target.writable) {
        // A write-protected stick is an expected case, not a failure worth
        // logging on every save.
        return false;
    }

    ScopedFsStore store(kLogTag);
    if (!store.open(target, kCreateTableDdl)) {
        return false;
    }

    QSqlQuery query(store.database());
    query.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO cue_overrides "
            "(relpath, version, updated_at, cues) "
            "VALUES (:relpath, :version, :updated_at, :cues)"));
    query.bindValue(QStringLiteral(":relpath"), target.relPath);
    query.bindValue(QStringLiteral(":version"), kPayloadVersion);
    query.bindValue(QStringLiteral(":updated_at"),
            QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":cues"), QString::fromUtf8(payload));
    if (!query.exec()) {
        qWarning() << "FsCueOverrideStore: cannot save cues for" << target.relPath
                   << query.lastError().text();
        return false;
    }
    qDebug() << "FsCueOverrideStore: saved cue override for" << target.relPath;
    return true;
}

// static
bool FsCueOverrideStore::clearFilesystemOverrides(const QString& mountPoint) {
    if (!flushPendingWrites()) {
        return false;
    }
    return fsStoreRemove(mountPoint, kStoreDbName, kLogTag);
}

// static
void FsCueOverrideStore::suppressPendingSaves() {
    QMutexLocker locker(&s_baselineMutex);
    for (auto it = s_baselines.begin(); it != s_baselines.end(); ++it) {
        *it = kSuppressedBaseline;
    }
}

// static
QStringList FsCueOverrideStore::overriddenLocations() {
    QMutexLocker locker(&s_baselineMutex);
    return s_importedCues.keys();
}

// static
bool FsCueOverrideStore::restoreImportedCues(Track* pTrack) {
    VERIFY_OR_DEBUG_ASSERT(pTrack) {
        return false;
    }
    QByteArray imported;
    {
        QMutexLocker locker(&s_baselineMutex);
        const auto it = s_importedCues.constFind(pTrack->getLocation());
        if (it == s_importedCues.constEnd()) {
            // No override was ever applied to this track, so it carries none of
            // this unit's cues and there is nothing for a clear to take off.
            return false;
        }
        imported = *it;
    }
    // An empty payload is meaningful here too: the source library exported no
    // cues at all, so every cue on the track is one the DJ added on this unit
    // and all of them go.
    applyPayload(pTrack, imported);
    return true;
}
