#include <cstdint>

#include "GameHost.hpp"

auto main(
    [[maybe_unused]] int32_t argc,
    [[maybe_unused]] char* argv[]) -> int32_t {

    TGameHost gameHost;
    gameHost.Run();
    return 0;
}