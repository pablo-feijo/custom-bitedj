#pragma once

#include <algorithm>
#include <cmath>

namespace mixxx {
// UI-only marquee: pause once at the beginning, then wrap through a second
// copy of the text. Elapsed time, rather than timer tick counts, keeps the
// speed stable when the UI misses a paint. Drawing two copies one cycle apart
// makes the wrap continuous instead of snapping from the end back to the start.
inline double textScrollOffset(double elapsedMs, double cycleDistance, double pixelsPerSecond) {
    if (cycleDistance <= 0 || pixelsPerSecond <= 0 || elapsedMs <= 0) {
        return 0;
    }
    constexpr double initialPauseMs = 1500;
    if (elapsedMs <= initialPauseMs) {
        return 0;
    }
    const double distance = (elapsedMs - initialPauseMs) * pixelsPerSecond / 1000;
    return std::fmod(distance, cycleDistance);
}
} // namespace mixxx
