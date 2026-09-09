#pragma once

#include <QAbstractTableModel>
#include <QThreadPool>
#include "library/libraryfeature.h"
#include "library/trackmodel.h"

class TrackCollectionManager;

// The queue owns track references in memory; only its portable metadata snapshot
// is written to disk, on a single worker, so edits never block on flash storage.
class PrepareModel final : public QAbstractTableModel, public TrackModel {
    Q_OBJECT
  public:
    PrepareModel(QObject* parent, const QString& path, TrackCollectionManager* collection = nullptr);
    ~PrepareModel() override;
    int rowCount(const QModelIndex& = {}) const override { return m_visible.size(); }
    int columnCount(const QModelIndex& = {}) const override { return 5; }
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int, Qt::Orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex&) const override;
    TrackPointer getTrack(const QModelIndex&) const override;
    TrackPointer getTrackByRef(const TrackRef&) const override;
    QUrl getTrackUrl(const QModelIndex& i) const override { return QUrl::fromLocalFile(getTrackLocation(i)); }
    QString getTrackLocation(const QModelIndex&) const override;
    TrackId getTrackId(const QModelIndex&) const override;
    TrackId getTrackRowIdentity(const QModelIndex&) const override;
    CoverInfo getCoverInfo(const QModelIndex&) const override;
    const QVector<int> getTrackRows(TrackId) const override;
    void search(const QString&) override;
    const QString currentSearch() const override { return m_search; }
    bool isColumnInternal(int) override { return false; }
    bool isColumnHiddenByDefault(int) override { return false; }
    bool isColumnSortable(int) const override { return false; }
    SortColumnId sortColumnIdFromColumnIndex(int) const override;
    int columnIndexFromSortColumnId(SortColumnId) const override;
    int fieldIndex(const QString&) const override;
    QString modelKey(bool) const override { return "bitedj.prepare"; }
    Capabilities getCapabilities() const override;
    QString getModelSetting(const QString&) override { return {}; }
    bool setModelSetting(const QString&, const QVariant&) override { return false; }
    bool isReady() const { return m_ready; }
    bool hasQueuedTracks() const { return !m_tracks.isEmpty(); }
    void add(const TrackPointerList&);
    void removeTracks(const QModelIndexList&) override;
    void moveTrack(const QModelIndex&, const QModelIndex&) override;
    void moveSelection(const QModelIndexList&, int direction);
    void clearQueue();
    bool updateTrackGenre(Track*, const QString&) const override { return false; }
#ifdef __EXTRA_METADATA__
    bool updateTrackMood(Track*, const QString&) const override { return false; }
#endif
  private:
    TrackPointer trackAt(const QModelIndex&) const;
    TrackCollectionManager* m_collection;
    void rebuild();
    void save();
    QString m_path, m_search;
    TrackPointerList m_tracks;
    QVector<int> m_visible;
    QThreadPool m_io;
    bool m_ready = false;
    QHash<QString, int> m_ids;
};

class PrepareFeature final : public LibraryFeature {
    Q_OBJECT
  public:
    PrepareFeature(Library*, UserSettingsPointer);
    QVariant title() override { return tr("Prepare"); }
    TreeItemModel* sidebarModel() const override { return m_tree; }
    bool hasTrackTable() override { return true; }
    bool isSidebarVisibleByDefault() const override { return m_model->hasQueuedTracks(); }
    void add(const TrackPointerList& tracks) { m_model->add(tracks); }
    void activate() override;
    void clear() override { m_model->clearQueue(); }
    bool dropAccept(const QList<QUrl>&, QObject*) override;
    bool dragMoveAccept(const QUrl&) override;
  private:
    PrepareModel* m_model;
    TreeItemModel* m_tree;
};
