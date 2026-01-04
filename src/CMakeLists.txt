add_custom_target(CopyData ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data
)

add_executable(OpenSpace
    Version.hpp
    Core.hpp
    Profiler.hpp
    FrameTimer.hpp
    FrameTimer.cpp
    Io.hpp
    Io.cpp
    Images.hpp
    Images.cpp
    Key.hpp
    Components.hpp
    Components.cpp
    Controls.hpp
    Assets.hpp
    Assets.cpp
    RHI.hpp
    RHI.cpp
    Renderer.hpp
    Renderer.cpp
    Scene.hpp
    Scene.cpp
    Main.cpp
)
add_dependencies(OpenSpace
    CopyData
)

target_compile_definitions(OpenSpace
    PRIVATE GLM_FORCE_CXX2A
    PRIVATE GLM_FORCE_RADIANS
    PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE
    PRIVATE GLM_FORCE_RIGHT_HANDED
    PRIVATE GLM_FORCE_SILENT_WARNINGS
    PRIVATE GLM_FORCE_SWIZZLE
    PRIVATE GLM_FORCE_XYZW_ONLY
    PRIVATE GLM_ENABLE_EXPERIMENTAL
)

target_precompile_headers(OpenSpace
    PUBLIC <array>
    PUBLIC <cassert>
    PUBLIC <chrono>
    PUBLIC "Core.hpp"
    PUBLIC <cstdint>
    PUBLIC <entt/entity/registry.hpp>
    PUBLIC <entt/entity/entity.hpp>
    PUBLIC <expected>
    PUBLIC <filesystem>
    PUBLIC <glm/gtc/quaternion.hpp>
    PUBLIC <glm/gtc/type_ptr.hpp>
    PUBLIC <glm/mat4x4.hpp>
    PUBLIC <glm/vec2.hpp>
    PUBLIC <glm/vec3.hpp>
    PUBLIC <glm/vec4.hpp>
    PUBLIC <iomanip>
    PUBLIC <memory>
    PUBLIC <optional>
    PUBLIC <parallel_hashmap/phmap.h>
    PUBLIC "Profiler.hpp"
    PUBLIC <ranges>
    PUBLIC <sstream>
    PUBLIC <stack>
    PUBLIC <string>
    PUBLIC <string_view>
    PUBLIC <utility>
    PUBLIC <vector>
)

target_link_libraries(OpenSpace
    PRIVATE mimalloc-static
    PRIVATE debugbreak
    PRIVATE poolSTL::poolSTL
    PRIVATE phmap

    PRIVATE spdlog

    PRIVATE glfw
    PRIVATE glad
    PRIVATE glm
    PRIVATE stb
    PRIVATE imgui
    PRIVATE imguizmo
    PRIVATE mikktspace
    PRIVATE fastgltf
    PRIVATE EnTT
    #PRIVATE Jolt::Jolt
    #PRIVATE ktx
)

# tbb is not required on windows for some reason
if(LINUX)
    message("Linking with libtbb")
    target_link_libraries(OpenSpace
        PRIVATE tbb
    )
endif()

target_sources(OpenSpace
    PRIVATE ${glm_SOURCE_DIR}/util/glm.natvis
)

if(USE_PROFILER)
    target_compile_definitions(OpenSpace
        PRIVATE USE_PROFILER
    )
    target_link_libraries(OpenSpace
        PRIVATE TracyClient
    )
endif()
