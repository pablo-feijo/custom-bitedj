#include "library/tabledelegates/previewbuttondelegate.h"

#include <QHeaderView>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

#include "control/controlobject.h"
#include "library/dao/analysisdao.h"
#include "library/dao/fsanalysiscache.h"
#include "library/trackmodel.h"
#include "moc_previewbuttondelegate.cpp"
#include "track/globaltrackcache.h"
#include "track/track.h"
#include "waveform/waveform.h"
#include "waveform/waveformfactory.h"
#include "widget/wlibrarytableview.h"
#include "widget/wtracktableview.h"

namespace {

inline TrackModel* trackModel(QTableView* pTableView) {
    VERIFY_OR_DEBUG_ASSERT(pTableView) {
        return nullptr;
    }
    return dynamic_cast<TrackModel*>(pTableView->model());
}

} // namespace

#include "control/controlproxy.h"

PreviewButtonDelegate::PreviewButtonDelegate(
        WLibraryTableView* parent,
        int column)
        : TableItemDelegate(parent),
          m_column(column) {
    DEBUG_ASSERT(m_column >= 0);
    m_pCOWaveformType = new ControlProxy(
            QStringLiteral("[Waveform]"), QStringLiteral("waveform_type"), this);
    m_pCOWaveformType->connectValueChanged(this, &PreviewButtonDelegate::waveformTypeChanged);
}

void PreviewButtonDelegate::waveformTypeChanged(double value) {
    Q_UNUSED(value);
    if (m_pTableView && m_pTableView->viewport()) {
        m_pTableView->viewport()->update();
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
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex& index) const {
    paintItemBackground(painter, option, index);

    TrackModel* const pTrackModel = trackModel(m_pTableView);
    if (!pTrackModel) {
        return;
    }
    
    // In BrowseTableModel, getTrack() calls getOrAddTrack() which hits the disk/database
    // and blocks the GUI thread. Since we only want to draw a waveform if it's readily
    // available, we use a non-blocking cache lookup instead.
    const TrackRef trackRef = TrackRef::fromFilePath(pTrackModel->getTrackLocation(index));
    TrackPointer pTrack = GlobalTrackCacheLocker().lookupTrackByRef(trackRef);
    
    if (!pTrack) {
        // Fallback: Check if the model is not BrowseTableModel (e.g. it's already in the library)
        // Actually, if it's in the library, it might be in cache. If not, we still don't want to block paint!
        // We can just show "LOAD" if it's not in cache.
        painter->setPen(QColor(150, 150, 160));
        QFont f = painter->font();
        f.setPointSizeF(f.pointSizeF() * 0.85);
        f.setBold(true);
        painter->setFont(f);
        painter->drawText(option.rect.adjusted(2, 2, -2, -2), Qt::AlignCenter, QStringLiteral("LOAD"));
        painter->restore();
        return;
    }

    const QRect cellRect = option.rect.adjusted(2, 2, -2, -2);
    if (cellRect.width() <= 4 || cellRect.height() <= 4) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPainterPath bgPath;
    bgPath.addRoundedRect(cellRect, 3, 3);
    painter->setBrush(QColor(24, 24, 32));
    painter->setPen(QPen(QColor(53, 53, 66), 1));
    painter->drawPath(bgPath);

    const int w = cellRect.width() - 4;
    const int h = cellRect.height() - 2;
    const int startX = cellRect.left() + 2;
    const int startY = cellRect.top() + 1;

    // Determine configured waveform type
    double wtVal = ControlObject::get(
            ConfigKey(QStringLiteral("[Waveform]"), QStringLiteral("waveform_type")));
    int waveformType = (wtVal > 0.0) ? static_cast<int>(wtVal) : 0;
    if (waveformType <= 0) {
        auto* pTableView = qobject_cast<WTrackTableView*>(m_pTableView);
        if (pTableView && pTableView->config()) {
            waveformType = pTableView->config()->getValue<int>(
                    ConfigKey(QStringLiteral("[Waveform]"), QStringLiteral("WaveformType")), 17);
        }
    }
    if (waveformType <= 0) {
        waveformType = 17; // Default: AllShaderRGBWaveform
    }

    const QString location = pTrack->getLocation();
    auto cacheIt = m_previewCache.find(location);
    if (cacheIt != m_previewCache.end() &&
        cacheIt->width == w &&
        cacheIt->height == h &&
        cacheIt->waveformType == waveformType) {
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->drawPixmap(startX, startY, cacheIt->pixmap);
        painter->restore();
        return;
    }

    // Retrieve waveform summary (or full waveform)
    ConstWaveformPointer pWaveform = pTrack->getWaveformSummary();
    if (!pWaveform) {
        pWaveform = pTrack->getWaveform();
    }
    if (!pWaveform) {
        auto* pTableView = qobject_cast<WTrackTableView*>(m_pTableView);
        if (pTableView && pTableView->config()) {
            FsAnalysisCache cache(pTableView->config());
            if (cache.isEnabled()) {
                QList<AnalysisDao::AnalysisInfo> analyses =
                        cache.getAnalysesForTrack(location);
                for (const auto& a : analyses) {
                    if (a.type == AnalysisDao::TYPE_WAVESUMMARY &&
                        WaveformFactory::waveformSummaryVersionToVersionClass(a.version) == WaveformFactory::VC_USE) {
                        pWaveform = ConstWaveformPointer(WaveformFactory::loadWaveformFromAnalysis(a));
                        pTrack->setWaveformSummary(pWaveform);
                        break;
                    }
                }
            }
            if (!pWaveform && pTrack->getId().isValid()) {
                AnalysisDao dao(pTableView->config());
                QList<AnalysisDao::AnalysisInfo> analyses = dao.getAnalysesForTrack(pTrack->getId());
                for (const auto& a : analyses) {
                    if (a.type == AnalysisDao::TYPE_WAVESUMMARY &&
                        WaveformFactory::waveformSummaryVersionToVersionClass(a.version) == WaveformFactory::VC_USE) {
                        pWaveform = ConstWaveformPointer(WaveformFactory::loadWaveformFromAnalysis(a));
                        pTrack->setWaveformSummary(pWaveform);
                        break;
                    }
                }
            }
        }
    }

    const int dataSize = pWaveform ? pWaveform->getDataSize() : 0;
    const int completion = pWaveform ? pWaveform->getCompletion() : 0;
    int validSamples = (completion > 0 && completion <= dataSize) ? completion : dataSize;
    int numFrames = validSamples / 2;

    if (numFrames <= 0) {
        painter->setPen(QColor(150, 150, 160));
        QFont f = painter->font();
        f.setPointSizeF(f.pointSizeF() * 0.85);
        f.setBold(true);
        painter->setFont(f);
        painter->drawText(cellRect, Qt::AlignCenter, QStringLiteral("LOAD"));
        painter->restore();
        return;
    }

    enum class WaveformStyle {
        RGB,
        Filtered,
        Stacked,
        HSV
    };
    WaveformStyle style = WaveformStyle::RGB;
    if (waveformType == 19 || waveformType == 2 || waveformType == 4 ||
        waveformType == 6 || waveformType == 7 || waveformType == 22) {
        style = WaveformStyle::Filtered;
    } else if (waveformType == 25 || waveformType == 26 || waveformType == 24 || waveformType == 16) {
        style = WaveformStyle::Stacked;
    } else if (waveformType == 8 || waveformType == 14 || waveformType == 21) {
        style = WaveformStyle::HSV;
    } else {
        style = WaveformStyle::RGB;
    }

    // Find peak amplitude across all frames for visual normalization
    unsigned char maxPeak = 0;
    for (int i = 0; i < numFrames; ++i) {
        const int leftIdx = 2 * i;
        const int rightIdx = 2 * i + 1;
        const unsigned char topAmp = pWaveform->getAll(leftIdx);
        const unsigned char botAmp = pWaveform->getAll(rightIdx);
        if (topAmp > maxPeak) {
            maxPeak = topAmp;
        }
        if (botAmp > maxPeak) {
            maxPeak = botAmp;
        }
    }
    if (maxPeak < 10) {
        maxPeak = 255;
    }

    const int centerY = h / 2;
    float scaleY = static_cast<float>(h) / static_cast<float>(2 * maxPeak + 2);
    
    // Draw directly into final size image
    QImage sourceImage(w, h, QImage::Format_ARGB32_Premultiplied);
    sourceImage.fill(Qt::transparent);

    QPainter imgPainter(&sourceImage);
    imgPainter.setRenderHint(QPainter::Antialiasing, false);

    // Baseline 
    imgPainter.setPen(QColor(0x35, 0x35, 0x42));
    imgPainter.drawLine(0, centerY, w, centerY);

    const QColor lowColor(0x00, 0x4e, 0xe4);   // Bass: blue
    const QColor midColor(0xc9, 0x5a, 0x00);   // Mid: amber
    const QColor highColor(0xf6, 0xe9, 0xd3);  // High: cream
    
    double step = static_cast<double>(numFrames) / w;

    for (int x = 0; x < w; ++x) {
        int i = static_cast<int>(x * step);
        if (i >= numFrames) i = numFrames - 1;
        
        const int leftIdx = 2 * i;
        const int rightIdx = 2 * i + 1;

        const int topAmp = static_cast<int>(pWaveform->getAll(leftIdx) * scaleY);
        const int botAmp = static_cast<int>(pWaveform->getAll(rightIdx) * scaleY);
        const int lowLeft = static_cast<int>(pWaveform->getLow(leftIdx) * scaleY);
        const int midLeft = static_cast<int>(pWaveform->getMid(leftIdx) * scaleY);
        const int highLeft = static_cast<int>(pWaveform->getHigh(leftIdx) * scaleY);
        const int lowRight = static_cast<int>(pWaveform->getLow(rightIdx) * scaleY);
        const int midRight = static_cast<int>(pWaveform->getMid(rightIdx) * scaleY);
        const int highRight = static_cast<int>(pWaveform->getHigh(rightIdx) * scaleY);

        if (style == WaveformStyle::RGB) {
            // Mixxx RGB overview: Low = Red, Mid = Green, High = Blue
            // Top half (Left channel)
            float rL = pWaveform->getLow(leftIdx);
            float gL = pWaveform->getMid(leftIdx);
            float bL = pWaveform->getHigh(leftIdx);
            float maxL = std::max({rL, gL, bL});
            if (maxL > 0.0f && topAmp > 0) {
                imgPainter.setPen(QColor::fromRgbF(rL / maxL, gL / maxL, bL / maxL));
                imgPainter.drawLine(x, centerY - topAmp, x, centerY);
            } else if (topAmp > 0) {
                imgPainter.setPen(QColor(0x00, 0x7d, 0xe1));
                imgPainter.drawLine(x, centerY - topAmp, x, centerY);
            }

            // Bottom half (Right channel)
            float rR = pWaveform->getLow(rightIdx);
            float gR = pWaveform->getMid(rightIdx);
            float bR = pWaveform->getHigh(rightIdx);
            float maxR = std::max({rR, gR, bR});
            if (maxR > 0.0f && botAmp > 0) {
                imgPainter.setPen(QColor::fromRgbF(rR / maxR, gR / maxR, bR / maxR));
                imgPainter.drawLine(x, centerY, x, centerY + botAmp);
            } else if (botAmp > 0) {
                imgPainter.setPen(QColor(0x00, 0x7d, 0xe1));
                imgPainter.drawLine(x, centerY, x, centerY + botAmp);
            }
        } else if (style == WaveformStyle::Filtered) {
            if (lowLeft > 0 || lowRight > 0) {
                imgPainter.setPen(lowColor);
                imgPainter.drawLine(x, centerY - lowLeft, x, centerY + lowRight);
            }
            if (midLeft > 0 || midRight > 0) {
                imgPainter.setPen(midColor);
                imgPainter.drawLine(x, centerY - midLeft, x, centerY + midRight);
            }
            if (highLeft > 0 || highRight > 0) {
                imgPainter.setPen(highColor);
                imgPainter.drawLine(x, centerY - highLeft, x, centerY + highRight);
            }
        } else if (style == WaveformStyle::Stacked) {
            const float stackScale = 0.6f;
            int hl = static_cast<int>(highLeft * stackScale);
            int hr = static_cast<int>(highRight * stackScale);
            int ml = static_cast<int>(midLeft * stackScale);
            int mr = static_cast<int>(midRight * stackScale);
            int ll = static_cast<int>(lowLeft * stackScale);
            int lr = static_cast<int>(lowRight * stackScale);

            if (hl > 0 || hr > 0) {
                imgPainter.setPen(highColor);
                imgPainter.drawLine(x, centerY - hl, x, centerY + hr);
            }
            if (ml > 0) {
                imgPainter.setPen(midColor);
                imgPainter.drawLine(x, centerY - hl - ml, x, centerY - hl);
            }
            if (mr > 0) {
                imgPainter.setPen(midColor);
                imgPainter.drawLine(x, centerY + hr, x, centerY + hr + mr);
            }
            if (ll > 0) {
                imgPainter.setPen(lowColor);
                imgPainter.drawLine(x, centerY - hl - ml - ll, x, centerY - hl - ml);
            }
            if (lr > 0) {
                imgPainter.setPen(lowColor);
                imgPainter.drawLine(x, centerY + hr + mr, x, centerY + hr + mr + lr);
            }
        } else if (style == WaveformStyle::HSV) {
            float total = (pWaveform->getLow(leftIdx) + pWaveform->getLow(rightIdx) + 
                           pWaveform->getMid(leftIdx) + pWaveform->getMid(rightIdx) + 
                           pWaveform->getHigh(leftIdx) + pWaveform->getHigh(rightIdx)) * 1.2f;
            float lo = 0.0f, hi = 0.0f;
            if (total > 0.0f) {
                lo = (pWaveform->getLow(leftIdx) + pWaveform->getLow(rightIdx)) / total;
                hi = (pWaveform->getHigh(leftIdx) + pWaveform->getHigh(rightIdx)) / total;
            }
            QColor hsvC;
            hsvC.setHsvF(0.6f, std::clamp(1.0f - hi, 0.0f, 1.0f), std::clamp(1.0f - lo, 0.0f, 1.0f));
            imgPainter.setPen(hsvC);
            imgPainter.drawLine(x, centerY - topAmp, x, centerY + botAmp);
        }
    }
    imgPainter.end();

    QPixmap resultPixmap = QPixmap::fromImage(sourceImage);

    CachedPreview cached;
    cached.pixmap = resultPixmap;
    cached.width = w;
    cached.height = h;
    cached.waveformType = waveformType;
    m_previewCache.insert(location, cached);

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->drawPixmap(startX, startY, resultPixmap);
    painter->restore();
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


