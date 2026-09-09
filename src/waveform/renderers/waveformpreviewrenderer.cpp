#include "waveform/renderers/waveformpreviewrenderer.h"

#include <QPainter>
#include <algorithm>
#include <array>
#include <cmath>

QImage WaveformPreviewRenderer::render(const ConstWaveformPointer& waveform,
        QSize size, int overviewType, const WaveformSignalColors& colors) {
    if (!waveform || size.width() < 1 || size.height() < 2) {
        return {};
    }
    const int frames = waveform->getDataSize() / 2;
    const int ready = std::clamp(waveform->getCompletion(), 0, waveform->getDataSize()) / 2;
    if (frames == 0 || ready == 0) {
        return {};
    }
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const double center = (size.height() - 1) / 2.0;
    // Stable gain while analyzing; normalize only once the summary is complete.
    double peak = 255.0;
    if (ready == frames) {
        peak = 1.0;
        for (int i = 0; i < ready * 2; ++i) {
            if (overviewType == 3) {
                if (i % 2 == 0) {
                    double sum = 0;
                    for (int c = 0; c < 2; ++c) {
                        sum += waveform->getLow(i + c) + waveform->getMid(i + c) +
                                waveform->getHigh(i + c);
                    }
                    peak = std::max(peak, sum / 3.0);
                }
            } else {
                peak = std::max(peak, double(waveform->getAll(i)));
            }
        }
    } else if (overviewType == 3) {
        peak = 510.0;
    }
    const double gain = (overviewType == 3 ? size.height() - 1 : center) / peak;
    const std::array<QColor, 3> rgb = {colors.getRgbLowColor(),
            colors.getRgbMidColor(), colors.getRgbHighColor()};
    const std::array<QColor, 3> filtered = {colors.getLowColor(),
            colors.getMidColor(), colors.getHighColor()};
    for (int x = 0; x < size.width(); ++x) {
        const int begin = static_cast<qint64>(x) * frames / size.width();
        if (begin >= ready) {
            break;
        }
        const int end = std::min(ready, std::max(begin + 1,
                int(static_cast<qint64>(x + 1) * frames / size.width())));
        // Preserve narrow transients when multiple summary frames share a pixel.
        int bands[2][3] = {};
        int all[2] = {};
        for (int f = begin; f < end; ++f) {
            for (int c = 0; c < 2; ++c) {
                const int i = 2 * f + c;
                all[c] = std::max(all[c], int(waveform->getAll(i)));
                bands[c][0] = std::max(bands[c][0], int(waveform->getLow(i)));
                bands[c][1] = std::max(bands[c][1], int(waveform->getMid(i)));
                bands[c][2] = std::max(bands[c][2], int(waveform->getHigh(i)));
            }
        }
        if (overviewType == 3) {
            double y = size.height() - 1;
            for (int b = 2; b >= 0; --b) {
                const double height = (bands[0][b] + bands[1][b]) / 3.0 * gain;
                if (height > 0) {
                    painter.setPen(rgb[b]);
                    painter.drawLine(QPointF(x, y), QPointF(x, std::max(0.0, y - height)));
                    y -= height;
                }
            }
        } else if (overviewType == 0) {
            for (int b = 0; b < 3; ++b) {
                if (bands[0][b] || bands[1][b]) {
                    painter.setPen(filtered[b]);
                    painter.drawLine(QPointF(x, center - bands[0][b] * gain),
                            QPointF(x, center + bands[1][b] * gain));
                }
            }
        } else {
            for (int c = 0; c < 2; ++c) {
                if (!all[c]) {
                    continue;
                }
                QColor color;
                if (overviewType == 1) {
                    const double total = std::max(1.0,
                            (bands[c][0] + bands[c][1] + bands[c][2]) * 1.2);
                    color = QColor::fromHsvF(0.6,
                            1.0 - bands[c][2] / total, 1.0 - bands[c][0] / total);
                } else {
                    double r = 0, g = 0, b = 0;
                    for (int band = 0; band < 3; ++band) {
                        r += bands[c][band] * rgb[band].redF();
                        g += bands[c][band] * rgb[band].greenF();
                        b += bands[c][band] * rgb[band].blueF();
                    }
                    const double maximum = std::max({r, g, b, 1.0});
                    color = QColor::fromRgbF(r / maximum, g / maximum, b / maximum);
                }
                painter.setPen(color);
                painter.drawLine(QPointF(x, center),
                        QPointF(x, center + (c ? 1 : -1) * all[c] * gain));
            }
        }
    }
    return image;
}
