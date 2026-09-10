#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include "waveform/waveform.h"
#include "util/fpclassify.h"

namespace mixxx::rekordbox {

// PWV6/PWV7 contain mono display heights in mid/high/low order, not PCM or
// stereo RMS. Preserve their relative heights and mirror the mono display into
// the renderer's two channels. Do not reinterpret Pioneer RGB bytes as bands.
inline std::vector<WaveformData> decodeThreeBandWaveform(const std::string& bytes,
        double sourceRate,
        double destinationRate,
        int destinationColumns,
        int timingOffsetMillis,
        bool normalizeDisplay = false) {
    if (bytes.empty() || bytes.size() % 3 != 0 ||
            bytes.size() / 3 > 4320000 || destinationColumns <= 0 ||
            destinationColumns > 4320002 ||
            !util_isfinite(sourceRate) || sourceRate <= 0 ||
            !util_isfinite(destinationRate) || destinationRate <= 0) {
        throw std::runtime_error("Invalid Rekordbox three-band waveform dimensions");
    }
    // Both PWV6 and PWV7 are prepared display envelopes, not native RMS.
    // Fit each complete envelope's shared band peak to the native renderers'
    // 8-bit display range. One fixed multiplier for the entire track preserves
    // relative band heights and dynamics, with no per-window gain pumping.
    // Existing skin/mode/visual gain controls still apply downstream. This is
    // display normalization, not PCM normalization or Pioneer's transfer curve.
    unsigned int peak = 0;
    if (normalizeDisplay) {
        for (unsigned char value : bytes) {
            peak = std::max(peak, static_cast<unsigned int>(value));
        }
    }
    const auto displayHeight = [&](char byte) -> unsigned char {
        const unsigned int value = static_cast<unsigned char>(byte);
        return normalizeDisplay && peak ? (value * 255u + peak / 2u) / peak : value;
    };
    std::vector<WaveformData> result(size_t(destinationColumns) * 2, WaveformData(0));
    for (int i = 0; i < destinationColumns; ++i) {
        const double sourceIndex = std::floor(
                i * (sourceRate / destinationRate) + timingOffsetMillis * (sourceRate / 1000.0));
        // Keep the 150 Hz detail timebase. Do not stretch it to rounded PDB
        // duration, or repeat the last column into audio beyond the export.
        if (sourceIndex < 0 || sourceIndex >= double(bytes.size() / 3)) {
            continue;
        }
        const size_t offset = size_t(sourceIndex) * 3;
        WaveformData value(0);
        value.filtered.mid = displayHeight(bytes[offset]);
        value.filtered.high = displayHeight(bytes[offset + 1]);
        value.filtered.low = displayHeight(bytes[offset + 2]);
        value.filtered.all = std::max({value.filtered.low, value.filtered.mid, value.filtered.high});
        result[size_t(i) * 2] = value;
        result[size_t(i) * 2 + 1] = value;
    }
    return result;
}

// PWV4 (.EXT) contains six bytes per preview column; bytes 3/4/5
// describe RGB levels, with byte 5 also giving the bright foreground height.
// PWV5 packs R/G/B (3 bits each) and height (5 bits) above two reserved bits.
// Format reference: https://djl-analysis.deepsymmetry.org/rekordbox-export-analysis/anlz.html
inline std::vector<WaveformRgb> decodeRgbWaveform(const std::string& bytes,
        bool overview, int destinationColumns, double sourceRate,
        double destinationRate, int timingOffsetMillis) {
    const size_t stride = overview ? 6 : 2;
    if (bytes.empty() || bytes.size() % stride || bytes.size() / stride > 4320000 ||
            destinationColumns <= 0 || destinationColumns > 4320002 ||
            !util_isfinite(sourceRate) || sourceRate <= 0 ||
            !util_isfinite(destinationRate) || destinationRate <= 0) {
        throw std::runtime_error("Invalid Rekordbox RGB waveform dimensions");
    }
    std::vector<WaveformRgb> result(destinationColumns);
    unsigned int peak = overview ? 1 : 31;
    if (overview) {
        for (size_t i = 0; i < bytes.size(); i += stride) {
            for (size_t c = 3; c < 6; ++c) peak = std::max(peak, unsigned(static_cast<unsigned char>(bytes[i + c])));
        }
    }
    for (int i = 0; i < destinationColumns; ++i) {
        const double index = std::floor(i * sourceRate / destinationRate +
                timingOffsetMillis * sourceRate / 1000.0);
        if (index < 0 || index >= double(bytes.size() / stride)) continue;
        const auto* b = reinterpret_cast<const unsigned char*>(bytes.data()) + size_t(index) * stride;
        auto& column = result[i];
        if (overview) {
            const unsigned int height = std::max({b[3], b[4], b[5]});
            if (!height) continue;
            column.red = b[3] * 255u / height;
            column.green = b[4] * 255u / height;
            column.blue = b[5] * 255u / height;
            column.height = height * 255u / peak;
            column.frontHeight = b[5] * 255u / peak;
        } else {
            const unsigned int packed = (unsigned(b[0]) << 8) | b[1];
            column.red = ((packed >> 13) & 7) * 255u / 7;
            column.green = ((packed >> 10) & 7) * 255u / 7;
            column.blue = ((packed >> 7) & 7) * 255u / 7;
            column.height = ((packed >> 2) & 31) * 255u / 31;
            column.frontHeight = column.height;
        }
    }
    return result;
}

} // namespace mixxx::rekordbox
