add_custom_target(copy_data ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data)

add_executable(HowToScript
    Main.cpp
)
add_dependencies(HowToScript copy_data)

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