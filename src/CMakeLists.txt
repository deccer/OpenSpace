add_custom_target(CopyData ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data
)

add_executable(HowToScript
    Io.cpp
    Images.cpp
    Helpers.cpp
    Components.cpp
    Assets.cpp
    RHI.cpp
    Renderer.cpp
    Scene.cpp
    Main.cpp
)
add_dependencies(HowToScript
    CopyData
)

target_compile_definitions(HowToScript
    PRIVATE GLM_FORCE_CXX2A
    PRIVATE GLM_FORCE_RADIANS
    PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE
    PRIVATE GLM_FORCE_RIGHT_HANDED
    PRIVATE GLM_FORCE_SILENT_WARNINGS
    PRIVATE GLM_FORCE_SWIZZLE
    PRIVATE GLM_FORCE_XYZW_ONLY
    PRIVATE GLM_ENABLE_EXPERIMENTAL
)

target_link_libraries(HowToScript
    PRIVATE glfw
    PRIVATE glad
    PRIVATE glm
    PRIVATE imgui
    PRIVATE imguizmo
    PRIVATE stb
    PRIVATE debugbreak
    PRIVATE spdlog
    PRIVATE poolSTL::poolSTL
    PRIVATE tbb
    PRIVATE fastgltf
    PRIVATE EnTT
    PRIVATE mimalloc-static
    PRIVATE Jolt
    PRIVATE mikktspace
)

target_sources(HowToScript
    PRIVATE ${glm_SOURCE_DIR}/util/glm.natvis
)

#target_link_options(HowToScript
#    PRIVATE /NATVIS:${glm_SOURCE_DIR}/util/glm.natvis
#)

if(USE_PROFILER)
    target_compile_definitions(HowToScript
        PRIVATE USE_PROFILER
    )
    target_link_libraries(HowToScript
        PRIVATE TracyClient
    )
endif()
