add_custom_target(CopyData ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data
)

# ---- LOGGER ----

add_library(Logger SHARED
    Core/Logger.cpp
)

# ---- ASSETS ----
add_subdirectory(Assets)

# ---- GAME ----
add_subdirectory(Game)

# --- GAMEHOST ---

add_executable(GameHost
    Core/Io.cpp
    Images.cpp
    GameHost.cpp
    Renderer/RHI/RHI.cpp
    Renderer/Renderer.cpp
    Main.cpp
)
add_dependencies(GameHost
    CopyData
    Logger
    Assets
    Game
)

target_compile_definitions(GameHost
    PRIVATE GLM_FORCE_CXX2A
    PRIVATE GLM_FORCE_RADIANS
    PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE
    PRIVATE GLM_FORCE_RIGHT_HANDED
    PRIVATE GLM_FORCE_SILENT_WARNINGS
    PRIVATE GLM_FORCE_SWIZZLE
    PRIVATE GLM_FORCE_XYZW_ONLY
    PRIVATE GLM_ENABLE_EXPERIMENTAL
)

target_precompile_headers(GameHost
    PUBLIC <chrono>
    PUBLIC <cstdint>
    PUBLIC <entt/entity/registry.hpp>
    PUBLIC <entt/entity/entity.hpp>
    PUBLIC <filesystem>
    PUBLIC <glm/mat4x4.hpp>
    PUBLIC <glm/gtc/quaternion.hpp>
    PUBLIC <glm/vec2.hpp>
    PUBLIC <glm/vec3.hpp>
    PUBLIC <glm/vec4.hpp>
    PUBLIC <memory>
    PUBLIC <string>
    PUBLIC <utility>
)

target_link_libraries(GameHost
    PRIVATE SDL2::SDL2-static
    PRIVATE glad
    PRIVATE glm
    PRIVATE imgui
    PRIVATE imguizmo
    PRIVATE stb
    PRIVATE debugbreak
    PRIVATE poolSTL::poolSTL
    PRIVATE phmap
    #PRIVATE tbb
    PRIVATE EnTT
    #PRIVATE mimalloc-static
    #PRIVATE Jolt
    PRIVATE mikktspace
    PRIVATE soloud

    PRIVATE Logger
    PRIVATE Assets
)

if(UNIX AND NOT APPLE)
    target_link_libraries(GameHost
        PRIVATE dl
    )
endif()

if(OPENSPACE_USE_PROFILER)
    target_compile_definitions(GameHost
        PRIVATE OPENSPACE_USE_PROFILER
    )
    target_link_libraries(GameHost
        PRIVATE TracyClient
    )
endif()
if(OPENSPACE_HOTRELOAD_GAME_ENABLED)
    target_compile_definitions(GameHost
        PRIVATE OPENSPACE_HOTRELOAD_GAME_ENABLED
    )
endif()
