include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            imgui
    GIT_REPOSITORY  https://github.com/ocornut/imgui
    GIT_TAG         docking
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
if(imgui_ADDED)
    add_library(imgui
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)

    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${glfw_SOURCE_DIR}/include)

    target_link_libraries(imgui
            PRIVATE glfw
    )
    if(WIN32 AND !APPLE)
        target_link_libraries(imgui
            PRIVATE X11
        )
    endif()
endif()

