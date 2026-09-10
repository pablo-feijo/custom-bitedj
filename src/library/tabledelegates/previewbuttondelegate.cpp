#include "library/tabledelegates/previewbuttondelegate.h"

#include <QDomDocument>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QPainter>
#include <QSqlQuery>
#include <QTimer>
#include <QThreadPool>
#include "util/rtscheduling.h"
#include <QtConcurrentRun>

#include "control/controlproxy.h"
#include "library/dao/analysisdao.h"
#include "library/dao/fsanalysiscache.h"
#include "library/trackmodel.h"
#include "library/rekordbox/rekordboxanlz.h"
#include "moc_previewbuttondelegate.cpp"
#include "track/globaltrackcache.h"
#include "track/track.h"
#include "util/db/dbconnectionpooled.h"
#include "util/db/dbconnectionpooler.h"
#include "util/painterscope.h"
#include "waveform/renderers/waveformpreviewrenderer.h"
#include "waveform/waveformfactory.h"
#include "waveform/widgets/waveformwidgettype.h"
#include "widget/wlibrarytableview.h"
#include "widget/wtracktableview.h"

namespace {
QThreadPool* summaryPool() {
    static QThreadPool pool;
    static const bool initialized = [] {
        pool.setMaxThreadCount(1);
        return true;
    }();
    Q_UNUSED(initialized);
    return &pool;
}
} // namespace

PreviewButtonDelegate::PreviewButtonDelegate(WLibraryTableView* parent, int column)
        : TableItemDelegate(parent), m_column(column) {
    DEBUG_ASSERT(m_column >= 0);
    m_pCOWaveformType = new ControlProxy(
            QStringLiteral("[Waveform]"), QStringLiteral("waveform_type"), this);
    m_pCOWaveformType->connectValueChanged(this,
            [this](double) { invalidatePreviews(); });
    auto* palette = new ControlProxy(ConfigKey("[BiteDJ]", "waveform_palette"),
            this, ControlFlag::NoAssertIfMissing);
    palette->connectValueChanged(this, [this](double value) {
        m_colors.applyBiteDJPalette(static_cast<int>(value));
        invalidatePreviews();
    });
    if (auto* view = qobject_cast<WTrackTableView*>(parent)) {
        SkinContext context(view->config(), "BiteDJ preview");
        QDomDocument xml;
        xml.setContent(QStringLiteral(
                "<Visual><BiteDJPalette>true</BiteDJPalette>"
                "<SignalColor>#32323c</SignalColor><SignalLowColor>#004ee4</SignalLowColor>"
                "<SignalMidColor>#c95a00</SignalMidColor>"
                "<SignalHighColor>#f6e9d3</SignalHighColor></Visual>"));
        m_colors.setup(xml.documentElement(), context);
    }
    auto* clear = new ControlProxy(ConfigKey("[Library]", "clear_cached_waveforms"),
            this, ControlFlag::NoAssertIfMissing);
    clear->connectValueChanged(this, [this](double value) {
        if (value > 0) {
            ++m_generation;
            m_summaries.clear();
            invalidatePreviews();
        }
    });
    if (parent->model()) {
        connect(parent->model(), &QAbstractItemModel::modelReset, this, [this] {
            ++m_generation;
            m_summaries.clear();
            invalidatePreviews();
        });
    }
    // Repaint only visible changed summaries. Analysis
    // writes completion atomically; it need not emit a signal for every block.
    auto* timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, this, &PreviewButtonDelegate::refreshVisiblePreviews);
    timer->start();
}

void PreviewButtonDelegate::invalidatePreviews() {
    m_previewCache.clear();
    if (m_pTableView) {
        m_pTableView->viewport()->update();
    }
}

ConstWaveformPointer PreviewButtonDelegate::summaryForLocation(const QString& location) const {
    const auto track = GlobalTrackCacheLocker().lookupPublishedTrackByLocation(location);
    auto* cached = m_summaries.object(location);
    if (track) {
        const auto waveform = track->getWaveformSummary();
        if (waveform) {
            // A native batch may still be running when this row is loaded.
            // Keep the already displayed export until the deck publishes its
            // exported summary, rather than flashing native analysis colors.
            if (cached && cached->waveform &&
                    cached->waveform->getVersion().startsWith("Rekordbox") &&
                    !waveform->getVersion().startsWith("Rekordbox") &&
                    !track->getRekordboxWaveformSource().analyzePath.isEmpty()) {
                return cached->waveform;
            }
            // Replace an earlier disk miss with the live analysis. Retain only
            // the summary so unloading the deck can still release the Track.
            if (!cached || cached->waveform != waveform ||
                    cached->sourceTrack.lock() != track) {
                m_summaries.insert(location, new CachedSummary{waveform, track});
            }
            return waveform;
        }
        // A clear on the same live Track wins even after a palette change has
        // discarded its pixmap. A newly imported metadata-only Track does not
        // invalidate the summary retained from a previous deck load.
        if (cached && cached->sourceTrack.lock() == track) {
            cached->waveform.clear();
        }
    }
    return cached ? cached->waveform : ConstWaveformPointer();
}

void PreviewButtonDelegate::requestSummary(const QString& location, const QString& analyzePath) const {
    if (m_requestPending || m_summaries.contains(location)) {
        return;
    }
    auto* view = qobject_cast<WTrackTableView*>(m_pTableView);
    if (!view) {
        return;
    }
    m_requestPending = true;
    const auto config = view->config();
    const auto pool = view->previewDbConnectionPool();
    auto* self = const_cast<PreviewButtonDelegate*>(this);
    auto* watcher = new QFutureWatcher<ConstWaveformPointer>(self);
    const int generation = m_generation;
    connect(watcher, &QFutureWatcher<ConstWaveformPointer>::finished, self,
            [self, watcher, location, generation] {
                self->m_requestPending = false;
                if (self->m_generation == generation &&
                        !self->m_summaries.contains(location)) {
                    self->m_summaries.insert(location,
                            new CachedSummary{watcher->result(), {}});
                    // Location-keyed result: sorting or changing folders during
                    // I/O cannot install one track's waveform into another row.
                }
                self->m_pTableView->viewport()->update();
                watcher->deleteLater();
            });
    watcher->setFuture(QtConcurrent::run(summaryPool(), [config, pool, location, analyzePath] {
        mixxx::demoteCurrentThreadToBackground("WaveformPreview");
        // This closure owns everything it uses. Destroying the view does not
        // wait for slow USB I/O and cannot leave a dangling delegate pointer.
        if (const auto exported = mixxx::rekordbox::readThreeBandPreview(analyzePath)) {
            return exported;
        }
        // Browsing must never create, migrate or repair files on the audio drive.
        FsAnalysisCache cache(config, FsAnalysisCache::AccessMode::ReadOnly);
        QList<AnalysisDao::AnalysisInfo> analyses;
        if (cache.isEnabled()) {
            analyses = cache.getAnalysesForTrack(location, AnalysisDao::TYPE_WAVESUMMARY);
        } else if (cache.isHomeCacheEnabled() && pool) {
            mixxx::DbConnectionPooler pooler(pool);
            if (pooler.isPooling()) {
                const QSqlDatabase db = mixxx::DbConnectionPooled(pool);
                QSqlQuery query(db);
                query.prepare(QStringLiteral("SELECT library.id FROM library "
                        "JOIN track_locations ON library.location = track_locations.id "
                        "WHERE track_locations.location = :location"));
                query.bindValue(QStringLiteral(":location"), location);
                if (query.exec() && query.next()) {
                    AnalysisDao dao(config);
                    dao.initialize(db);
                    analyses = dao.getAnalysesForTrackByType(
                            TrackId(query.value(0)), AnalysisDao::TYPE_WAVESUMMARY);
                }
            }
        }
        for (const auto& analysis : analyses) {
            if (WaveformFactory::waveformSummaryVersionToVersionClass(analysis.version) ==
                    WaveformFactory::VC_USE) {
                return ConstWaveformPointer(WaveformFactory::loadWaveformFromAnalysis(analysis));
            }
        }
        return ConstWaveformPointer();
    }));
}

void PreviewButtonDelegate::refreshVisiblePreviews() {
    // A previously missing track may be loaded while another page is open.
    // Promote its live summary before the deck releases it, without disk I/O
    // or importing tracks. The visited-location cache is bounded to 128 entries.
    for (const auto& location : m_summaries.keys()) {
        summaryForLocation(location);
    }
    if (!m_pTableView->isVisible() || m_pTableView->isColumnHidden(m_column)) {
        return;
    }
    auto* model = dynamic_cast<TrackModel*>(m_pTableView->model());
    if (!model) {
        return;
    }
    const int first = m_pTableView->rowAt(0);
    if (first < 0) {
        return;
    }
    for (int row = first; row < m_pTableView->model()->rowCount(); ++row) {
        const QModelIndex index = m_pTableView->model()->index(row, m_column);
        const QRect rect = m_pTableView->visualRect(index);
        if (rect.top() >= m_pTableView->viewport()->height()) {
            break;
        }
        const QString location = model->getTrackLocation(index);
        const auto waveform = summaryForLocation(location);
        const auto* cached = m_previewCache.object(location);
        if ((waveform && (!cached || cached->waveform != waveform ||
                                 cached->completion != waveform->getCompletion())) ||
                (!waveform && cached)) {
            m_pTableView->viewport()->update(rect);
        }
    }
}

PreviewButtonDelegate::~PreviewButtonDelegate() = default;

QWidget* PreviewButtonDelegate::createEditor(QWidget* parent,
                                             const QStyleOptionViewItem& option,
                                             const QModelIndex& index) const {
    Q_UNUSED(parent);
    Q_UNUSED(option);
    Q_UNUSED(index);
    return nullptr;
}

void PreviewButtonDelegate::setEditorData(QWidget* editor,
                                          const QModelIndex& index) const {
    Q_UNUSED(editor);
    Q_UNUSED(index);
}

void PreviewButtonDelegate::setModelData(QWidget* editor,
                                         QAbstractItemModel* model,
                                         const QModelIndex& index) const {
    Q_UNUSED(editor);
    Q_UNUSED(model);
    Q_UNUSED(index);
}

void PreviewButtonDelegate::paintItem(QPainter* painter,
        const QStyleOptionViewItem& option, const QModelIndex& index) const {
    PainterScope scope(painter);
    paintItemBackground(painter, option, index);
    auto* model = dynamic_cast<TrackModel*>(m_pTableView->model());
    if (!model) {
        return;
    }
    const QRect rect = option.rect.adjusted(4, 3, -4, -3);
    if (rect.width() < 1 || rect.height() < 2) {
        return;
    }
    const QString location = model->getTrackLocation(index);
    const auto waveform = summaryForLocation(location);
    if (!waveform) {
        const int analysisColumn = model->fieldIndex(QStringLiteral("analyze_path"));
        const QString analyzePath = analysisColumn >= 0
                ? index.sibling(index.row(), analysisColumn).data(Qt::EditRole).toString()
                : QString();
        requestSummary(location, analyzePath);
    }
    const int completion = waveform ? waveform->getCompletion() : 0;
    auto* cached = m_previewCache.object(location);
    if (!cached || cached->waveform != waveform || cached->completion != completion ||
            cached->pixmap.size() != rect.size()) {
        const auto type = static_cast<WaveformWidgetType::Type>(int(m_pCOWaveformType->get()));
        const QImage image = WaveformPreviewRenderer::render(waveform, rect.size(),
                WaveformWidgetType::overviewType(type), m_colors);
        if (!image.isNull()) {
            cached = new CachedPreview{QPixmap::fromImage(image), waveform, completion};
            m_previewCache.insert(location, cached);
        } else {
            m_previewCache.remove(location);
            cached = nullptr;
        }
    }
    if (cached) {
        painter->drawPixmap(rect.topLeft(), cached->pixmap);
    } else {
        painter->setPen(option.palette.color(QPalette::Disabled, QPalette::Text));
        painter->drawText(rect, Qt::AlignCenter, QStringLiteral("—"));
    }
}

void PreviewButtonDelegate::updateEditorGeometry(QWidget* editor,
                                                 const QStyleOptionViewItem& option,
                                                 const QModelIndex& index) const {
    Q_UNUSED(editor);
    Q_UNUSED(option);
    Q_UNUSED(index);
}

QSize PreviewButtonDelegate::sizeHint(const QStyleOptionViewItem& option,
                                      const QModelIndex& index) const {
    Q_UNUSED(option);
    Q_UNUSED(index);
    int h = 22;
    if (m_pTableView && m_pTableView->verticalHeader()) {
        h = m_pTableView->verticalHeader()->defaultSectionSize();
    }
    return QSize(96, h);
}

void PreviewButtonDelegate::cellEntered(const QModelIndex& index) {
    Q_UNUSED(index);
}


