#pragma once

#include <cstdint>

enum class TWindowStyle {
    Windowed,
    Fullscreen,
    FullscreenExclusive
};

struct TWindowSettings {
    int32_t ResolutionWidth;
    int32_t ResolutionHeight;
    float ResolutionScale;
    TWindowStyle WindowStyle;
    bool IsDebug;
    bool IsVSyncEnabled;
};
