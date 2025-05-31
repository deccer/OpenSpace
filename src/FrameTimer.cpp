#include "FrameTimer.hpp"
#include <poolstl/algorithm>

static constexpr size_t MaxSamples = 256;
auto g_frameTimes = std::vector<float>(MaxSamples, 0.0f);
std::chrono::high_resolution_clock::time_point g_lastTime = {};
bool g_isInitialized = false;

auto FrameTimer::FrameStart() -> void {

    const auto now = std::chrono::high_resolution_clock::now();
    if (g_isInitialized) {
        const float frameTime = std::chrono::duration<float, std::milli>(now - g_lastTime).count();
        g_frameTimes.push_back(frameTime);
        if (g_frameTimes.size() > MaxSamples) {
            g_frameTimes.erase(g_frameTimes.begin());
        }
    } else {
        g_isInitialized = true;
    }
    g_lastTime = now;
}

auto GetPercentileFPS(const float percentile) -> float {
    if (g_frameTimes.empty()) {
        return 0.0f;
    }
    std::vector<float> sorted = g_frameTimes;
    std::ranges::sort(sorted.begin(), sorted.end(), std::greater<float>());
    size_t index = percentile * sorted.size();
    index = std::min(index, sorted.size() - 1);
    return 1000.0f / sorted[index];
}

auto FrameTimer::GetCurrentFPS() -> float {
    if (g_frameTimes.empty()) {
        return 0.0f;
    }
    return 1000.0f / g_frameTimes.back();
}

auto FrameTimer::GetAverageFrameTime() -> float {
    if (g_frameTimes.empty()) {
        return 0.0f;
    }
    float sum = 0.0f;
    for (const auto t : g_frameTimes) {
        sum += t;
    }
    return sum / g_frameTimes.size();
}

auto FrameTimer::Get1PercentLow() -> float {
    return GetPercentileFPS(0.01f);
}

auto FrameTimer::Get01PercentLow() -> float {
    return GetPercentileFPS(0.001f);
}

