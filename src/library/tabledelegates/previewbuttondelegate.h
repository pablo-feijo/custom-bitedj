#pragma once

#include "library/tabledelegates/tableitemdelegate.h"
#include "track/track_decl.h"

#include <QHash>
#include <QPixmap>

class WLibraryTableView;

class PreviewButtonDelegate : public TableItemDelegate {
    Q_OBJECT

  public:
    PreviewButtonDelegate(WLibraryTableView* parent, int column);
    ~PreviewButtonDelegate() override;

    QWidget* createEditor(
            QWidget* parent,
            const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;

    void setEditorData(
            QWidget* editor,
            const QModelIndex& index) const override;
    void setModelData(
            QWidget* editor,
            QAbstractItemModel* model,
            const QModelIndex& index) const override;
    void paintItem(
            QPainter* painter,
            const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;
    QSize sizeHint(
            const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;
    void updateEditorGeometry(
            QWidget* editor,
            const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;

  signals:
    void loadTrackToPlayer(const TrackPointer& pTrack, const QString& group, bool play);

  public slots:
    void cellEntered(const QModelIndex& index);

  private slots:
    void waveformTypeChanged(double value);

  private:
    struct CachedPreview {
        QPixmap pixmap;
        int width;
        int height;
        int waveformType;
    };

    const int m_column;
    mutable QHash<QString, CachedPreview> m_previewCache;
    class ControlProxy* m_pCOWaveformType;
};
