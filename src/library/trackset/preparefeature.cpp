#include "library/trackset/preparefeature.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include "library/library.h"
#include "library/trackcollectionmanager.h"
#include "library/treeitem.h"
#include "sources/soundsourceproxy.h"
#include "track/track.h"
#include "moc_preparefeature.cpp"

namespace {
const QStringList fields{"position", "artist", "title", "duration", "bpm"};
const TrackModel::SortColumnId sorts[]{TrackModel::SortColumnId::Position,
    TrackModel::SortColumnId::Artist, TrackModel::SortColumnId::Title,
    TrackModel::SortColumnId::Duration, TrackModel::SortColumnId::Bpm};
TrackPointer temporary(const QString& path) {
    return Track::newTemporary(mixxx::FileAccess(mixxx::FileInfo(path)));
}
}
PrepareModel::PrepareModel(QObject* parent, const QString& path, TrackCollectionManager* collection)
        : QAbstractTableModel(parent), TrackModel(QSqlDatabase(), "bitedj.prepare"), m_collection(collection), m_path(path) {
    m_io.setMaxThreadCount(1);
    m_io.start([this, path] {
        QFile file(path);
        QJsonArray rows;
        if (file.open(QIODevice::ReadOnly) && file.size() <= 4 * 1024 * 1024)
            rows = QJsonDocument::fromJson(file.readAll()).array();
        QMetaObject::invokeMethod(this, [this, rows] {
            TrackPointerList restored;
            for (const auto& value : rows) {
                const auto row = value.toObject();
                const auto path = row.value("path").toString();
                if (!QDir::isAbsolutePath(path)) continue;
                auto track = temporary(path);
                track->setTitle(row.value("title").toString());
                track->setArtist(row.value("artist").toString());
                const int rate = row.value("sampleRate").toInt();
                const int channels = row.value("channels").toInt();
                const double duration = row.value("duration").toDouble();
                if (rate > 0 && channels > 0 && duration > 0) {
                    track->setAudioProperties(mixxx::audio::ChannelCount(channels),
                            mixxx::audio::SampleRate(rate), mixxx::audio::Bitrate(),
                            mixxx::Duration::fromSeconds(duration));
                }
                const double bpm = row.value("bpm").toDouble();
                if (bpm > 0) track->trySetBpm(bpm);
                track->markClean();
                restored.append(track);
            }
            // Restore saved ordering before any tracks queued during startup.
            const auto pending = m_tracks;
            m_tracks.clear();
            add(restored);
            m_ready = true;
            add(pending);
        }, Qt::QueuedConnection);
    });
}
PrepareModel::~PrepareModel() { m_io.waitForDone(); }
TrackPointer PrepareModel::trackAt(const QModelIndex& i) const {
    return i.isValid() && i.row() >= 0 && i.row() < m_visible.size()
            ? m_tracks.at(m_visible.at(i.row())) : TrackPointer();
}
TrackPointer PrepareModel::getTrack(const QModelIndex& i) const {
    auto track = trackAt(i);
    if (track && m_collection) {
        // Only on an explicit load/menu request, never during cell painting.
        // Reuse library cue/beat metadata when restoring a saved queue.
        auto known = m_collection->getTrackByRef(TrackRef::fromFilePath(track->getLocation()));
        if (known) return known;
    }
    return track;
}
TrackPointer PrepareModel::getTrackByRef(const TrackRef& ref) const {
    for (const auto& t : m_tracks) if (t->getLocation() == ref.getLocation()) return t;
    return {};
}
QString PrepareModel::getTrackLocation(const QModelIndex& i) const { auto t=trackAt(i); return t?t->getLocation():QString(); }
TrackId PrepareModel::getTrackId(const QModelIndex& i) const { auto t=trackAt(i); return t?t->getId():TrackId(); }
TrackId PrepareModel::getTrackRowIdentity(const QModelIndex& i) const { return TrackId(QVariant(m_ids.value(getTrackLocation(i), -1))); }
CoverInfo PrepareModel::getCoverInfo(const QModelIndex& i) const { auto t=trackAt(i); return t?t->getCoverInfoWithLocation():CoverInfo(); }
const QVector<int> PrepareModel::getTrackRows(TrackId id) const {
    QVector<int> rows;
    for(int i=0;i<rowCount();++i) if(getTrackId(index(i,0))==id) rows.append(i);
    return rows;
}
QVariant PrepareModel::data(const QModelIndex& i, int role) const {
    auto t=trackAt(i); if(!t) return {};
    if(role==Qt::ToolTipRole) return t->getLocation();
    if(role!=Qt::DisplayRole && role!=Qt::EditRole) return {};
    switch(i.column()) {
    case 0: return m_visible.at(i.row())+1;
    case 1: return t->getArtist();
    case 2: return t->getTitle().isEmpty()?QFileInfo(t->getLocation()).completeBaseName():t->getTitle();
    case 3: { const int seconds=int(t->getDuration()); return seconds>0?QString("%1:%2").arg(seconds/60).arg(seconds%60,2,10,QChar('0')):QString(); }
    case 4: return t->getBpm()>0?QString::number(t->getBpm(),'f',1):QString();
    default: return {};
    }
}
QVariant PrepareModel::headerData(int n, Qt::Orientation o, int role) const {
    if(o!=Qt::Horizontal || n<0 || n>=5) return {};
    if(role==kHeaderNameRole) return fields[n];
    if(role==kHeaderWidthRole) return n==2?360:(n==1?260:80);
    if(role==Qt::DisplayRole) return QStringList{tr("#"),tr("Artist"),tr("Title"),tr("Time"),tr("BPM")}[n];
    return {};
}
Qt::ItemFlags PrepareModel::flags(const QModelIndex& i) const { return i.isValid()?Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsDragEnabled:Qt::NoItemFlags; }
TrackModel::Capabilities PrepareModel::getCapabilities() const { return Capability::LoadToDeck|Capability::Remove; }
TrackModel::SortColumnId PrepareModel::sortColumnIdFromColumnIndex(int i) const { return i>=0&&i<5?sorts[i]:SortColumnId::Invalid; }
int PrepareModel::columnIndexFromSortColumnId(SortColumnId id) const { for(int i=0;i<5;++i) if(sorts[i]==id) return i; return -1; }
int PrepareModel::fieldIndex(const QString& f) const { return fields.indexOf(f); }
void PrepareModel::rebuild() {
    beginResetModel(); m_visible.clear();
    for(int i=0;i<m_tracks.size();++i) {
        const auto& t=m_tracks[i];
        if(QString(t->getTitle()+" "+t->getArtist()+" "+t->getLocation()).contains(m_search,Qt::CaseInsensitive)) m_visible.append(i);
    }
    endResetModel();
}
void PrepareModel::search(const QString& text) { m_search=text; rebuild(); }
void PrepareModel::add(const TrackPointerList& tracks) {
    QSet<QString> seen;
    bool changed = false;
    for(const auto& t:m_tracks) seen.insert(t->getLocation());
    for(const auto& t:tracks) {
        if(!t || seen.contains(t->getLocation()) || m_tracks.size()>=10000) continue;
        seen.insert(t->getLocation()); m_tracks.append(t); changed = true;
        if(!m_ids.contains(t->getLocation())) m_ids.insert(t->getLocation(),m_ids.size()+1);
    }
    rebuild(); if(m_ready && changed) save();
}
void PrepareModel::removeTracks(const QModelIndexList& indices) {
    QSet<QString> paths; for(const auto& i:indices) paths.insert(getTrackLocation(i));
    m_tracks.removeIf([&paths](const auto& t){ return paths.contains(t->getLocation()); });
    rebuild(); save();
}
void PrepareModel::moveTrack(const QModelIndex& from,const QModelIndex& to) {
    if(!trackAt(from)||!trackAt(to)) return;
    m_tracks.move(m_visible[from.row()],m_visible[to.row()]); rebuild(); save();
}
void PrepareModel::moveSelection(const QModelIndexList& indices,int direction) {
    if(indices.size()!=1) return;
    const auto i=indices.first(); moveTrack(i,index(i.row()+direction,0));
}
void PrepareModel::clearQueue() { m_tracks.clear(); rebuild(); save(); }
void PrepareModel::save() {
    if(!m_ready) return;
    QJsonArray rows;
    for(const auto& t:m_tracks) rows.append(QJsonObject{{"path",t->getLocation()},{"title",t->getTitle()},{"artist",t->getArtist()},{"sampleRate",int(t->getSampleRate().value())},{"channels",t->getChannels()},{"duration",t->getDuration()},{"bpm",t->getBpm()}});
    const auto bytes=QJsonDocument(rows).toJson(QJsonDocument::Compact); const auto path=m_path;
    m_io.start([path,bytes] {
        QDir().mkpath(QFileInfo(path).absolutePath()); QSaveFile file(path);
        if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit())
            qWarning()<<"Could not save Prepare queue:"<<path;
    });
}
PrepareFeature::PrepareFeature(Library* lib,UserSettingsPointer config)
        : LibraryFeature(lib,config,QStringLiteral("playlist")),
          m_model(new PrepareModel(this,QDir(config->getSettingsPath()).filePath("prepare.json"),lib->trackCollectionManager())),
          m_tree(new TreeItemModel(this)) { m_tree->setRootItem(TreeItem::newRoot(this)); }
void PrepareFeature::activate() { emit saveModelState(); emit showTrackModel(m_model); emit enableCoverArtDisplay(false); }
bool PrepareFeature::dragMoveAccept(const QUrl& url) { return SoundSourceProxy::isUrlSupported(url); }
bool PrepareFeature::dropAccept(const QList<QUrl>& urls,QObject*) {
    TrackPointerList tracks; for(const auto& url:urls) if(url.isLocalFile()&&dragMoveAccept(url)) tracks.append(temporary(url.toLocalFile()));
    m_model->add(tracks); return !tracks.isEmpty();
}
