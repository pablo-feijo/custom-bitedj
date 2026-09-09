#pragma once

#include "library/tabledelegates/tableitemdelegate.h"
#include "track/track_decl.h"
#include "waveform/waveform.h"

#include <QCache>
#include "waveform/renderers/waveformsignalcolors.h"
#include <QPixmap>

class WLibraryTableView;

class PreviewButtonDelegate : public TableItemDelegate {
    friend class PreviewDelegateTest;
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

  private:
    struct CachedPreview {
        QPixmap pixmap;
        ConstWaveformPointer waveform;
        int completion = 0;
        bool fromLiveTrack = false;
    };
    void refreshVisiblePreviews();
    void invalidatePreviews();
    void requestSummary(const QString& location) const;
    ConstWaveformPointer summaryForLocation(const QString& location) const;

    const int m_column;
    mutable QCache<QString, CachedPreview> m_previewCache{128};
    // A null value is a cached miss, so an uncached track does not repeatedly
    // open its filesystem on every repaint. Loading analysis into a deck wins.
    mutable QCache<QString, ConstWaveformPointer> m_summaries{128};
    mutable bool m_requestPending = false;
    int m_generation = 0;
    WaveformSignalColors m_colors;
    class ControlProxy* m_pCOWaveformType;
};
