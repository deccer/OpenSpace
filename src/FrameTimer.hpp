#pragma once

namespace FrameTimer {
    auto FrameStart() -> void;
    auto GetAverageFrameTime() -> float;
    auto GetCurrentFPS() -> float;
    auto Get1PercentLow() -> float;
    auto Get01PercentLow() -> float;
}
