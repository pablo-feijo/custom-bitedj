#pragma once

#include <QImage>
#include <QSize>

#include "waveform/renderers/waveformsignalcolors.h"
#include "waveform/waveform.h"

// Summary-only renderer. Completion is a published prefix; its position is
// always measured against the full track, including while analysis is running.
class WaveformPreviewRenderer {
  public:
    static QImage render(const ConstWaveformPointer& waveform,
            QSize size, int overviewType, const WaveformSignalColors& colors);
};
