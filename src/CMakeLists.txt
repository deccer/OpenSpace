add_custom_target(CopyData ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data)

add_executable(HowToScript
    Main.cpp
)
add_dependencies(HowToScript CopyData)

target_compile_definitions(HowToScript
    PRIVATE GLM_FORCE_CXX2A
    PRIVATE GLM_FORCE_RADIANS
    PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE
    PRIVATE GLM_FORCE_RIGHT_HANDED
    PRIVATE GLM_FORCE_SILENT_WARNINGS
    PRIVATE GLM_FORCE_SWIZZLE
    PRIVATE GLM_ENABLE_EXPERIMENTAL
)

target_compile_definitions(HowToScript
    PRIVATE USE_PROFILER
)

target_link_libraries(HowToScript
    PRIVATE glfw
    PRIVATE glad
    PRIVATE glm
    PRIVATE imgui
    PRIVATE stb
    PRIVATE debugbreak
    PRIVATE spdlog
    PRIVATE poolSTL::poolSTL
    PRIVATE tbb
    PRIVATE TracyClient
    PRIVATE fastgltf
    PRIVATE EnTT
)