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

PreviewButtonDelegate::PreviewButtonDelegate(
        WLibraryTableView* parent,
        int column)
        : TableItemDelegate(parent),
          m_column(column) {
    DEBUG_ASSERT(m_column >= 0);
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
    TrackPointer pTrack = pTrackModel->getTrack(index);
    if (!pTrack) {
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

    const int srcH = 2 * static_cast<int>(maxPeak) + 2;
    const int centerY = srcH / 2;
    
    // Always use numFrames so that whatever data we have ALWAYS stretches to fill the entire preview box!
    QImage sourceImage(numFrames, srcH, QImage::Format_ARGB32_Premultiplied);
    sourceImage.fill(Qt::transparent);

    QPainter imgPainter(&sourceImage);
    imgPainter.setRenderHint(QPainter::Antialiasing, false);

    // Baseline 
    imgPainter.setPen(QColor(0x35, 0x35, 0x42));
    imgPainter.drawLine(0, centerY, numFrames, centerY);

    const QColor lowColor(0x00, 0x4e, 0xe4);   // Bass: blue
    const QColor midColor(0xc9, 0x5a, 0x00);   // Mid: amber
    const QColor highColor(0xf6, 0xe9, 0xd3);  // High: cream

    for (int i = 0; i < numFrames; ++i) {
        const int leftIdx = 2 * i;
        const int rightIdx = 2 * i + 1;

        const unsigned char topAmp = pWaveform->getAll(leftIdx);
        const unsigned char botAmp = pWaveform->getAll(rightIdx);
        const unsigned char lowLeft = pWaveform->getLow(leftIdx);
        const unsigned char midLeft = pWaveform->getMid(leftIdx);
        const unsigned char highLeft = pWaveform->getHigh(leftIdx);
        const unsigned char lowRight = pWaveform->getLow(rightIdx);
        const unsigned char midRight = pWaveform->getMid(rightIdx);
        const unsigned char highRight = pWaveform->getHigh(rightIdx);

        if (style == WaveformStyle::RGB) {
            // Mixxx RGB overview: Low = Red, Mid = Green, High = Blue
            // Top half (Left channel)
            float rL = lowLeft;
            float gL = midLeft;
            float bL = highLeft;
            float maxL = std::max({rL, gL, bL});
            if (maxL > 0.0f && topAmp > 0) {
                imgPainter.setPen(QColor::fromRgbF(rL / maxL, gL / maxL, bL / maxL));
                imgPainter.drawLine(i, centerY - topAmp, i, centerY);
            } else if (topAmp > 0) {
                imgPainter.setPen(QColor(0x00, 0x7d, 0xe1));
                imgPainter.drawLine(i, centerY - topAmp, i, centerY);
            }

            // Bottom half (Right channel)
            float rR = lowRight;
            float gR = midRight;
            float bR = highRight;
            float maxR = std::max({rR, gR, bR});
            if (maxR > 0.0f && botAmp > 0) {
                imgPainter.setPen(QColor::fromRgbF(rR / maxR, gR / maxR, bR / maxR));
                imgPainter.drawLine(i, centerY, i, centerY + botAmp);
            } else if (botAmp > 0) {
                imgPainter.setPen(QColor(0x00, 0x7d, 0xe1));
                imgPainter.drawLine(i, centerY, i, centerY + botAmp);
            }
        } else if (style == WaveformStyle::Filtered) {
            // Layered 3-band: Low background, Mid layer, High foreground
            if (lowLeft > 0 || lowRight > 0) {
                imgPainter.setPen(lowColor);
                imgPainter.drawLine(i, centerY - lowLeft, i, centerY + lowRight);
            }
            if (midLeft > 0 || midRight > 0) {
                imgPainter.setPen(midColor);
                imgPainter.drawLine(i, centerY - midLeft, i, centerY + midRight);
            }
            if (highLeft > 0 || highRight > 0) {
                imgPainter.setPen(highColor);
                imgPainter.drawLine(i, centerY - highLeft, i, centerY + highRight);
            }
        } else if (style == WaveformStyle::Stacked) {
            // Stacked 3-band from center outwards
            if (lowLeft > 0 || lowRight > 0) {
                imgPainter.setPen(lowColor);
                imgPainter.drawLine(i, centerY - lowLeft, i, centerY + lowRight);
            }
            if (midLeft > 0) {
                imgPainter.setPen(midColor);
                imgPainter.drawLine(i, centerY - lowLeft - midLeft, i, centerY - lowLeft);
            }
            if (midRight > 0) {
                imgPainter.setPen(midColor);
                imgPainter.drawLine(i, centerY + lowRight, i, centerY + lowRight + midRight);
            }
            if (highLeft > 0) {
                imgPainter.setPen(highColor);
                imgPainter.drawLine(i, centerY - lowLeft - midLeft - highLeft, i, centerY - lowLeft - midLeft);
            }
            if (highRight > 0) {
                imgPainter.setPen(highColor);
                imgPainter.drawLine(i, centerY + lowRight + midRight, i, centerY + lowRight + midRight + highRight);
            }
        } else if (style == WaveformStyle::HSV) {
            float total = (lowLeft + lowRight + midLeft + midRight + highLeft + highRight) * 1.2f;
            float lo = 0.0f, hi = 0.0f;
            if (total > 0.0f) {
                lo = (lowLeft + lowRight) / total;
                hi = (highLeft + highRight) / total;
            }
            QColor hsvC;
            hsvC.setHsvF(0.6f, std::clamp(1.0f - hi, 0.0f, 1.0f), std::clamp(1.0f - lo, 0.0f, 1.0f));
            imgPainter.setPen(hsvC);
            imgPainter.drawLine(i, centerY - topAmp, i, centerY + botAmp);
        }
    }
    imgPainter.end();

    QImage scaledImg = sourceImage.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPixmap resultPixmap = QPixmap::fromImage(scaledImg);

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


