#include "GameHost.hpp"
#include "WindowSettings.hpp"

auto main(
    [[maybe_unused]] int32_t argc,
    [[maybe_unused]] char* argv[]) -> int32_t {

    auto windowSettings = TWindowSettings{
        .ResolutionWidth = 1920,
        .ResolutionHeight = 1080,
        .ResolutionScale = 1.0f,
        .WindowStyle = TWindowStyle::Windowed,
        .IsDebug = true,
        .IsVSyncEnabled = true
    };

    TGameHost gameHost;
    gameHost.Run(&windowSettings);
    return 0;
}
