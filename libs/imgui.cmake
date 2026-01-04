include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            imgui
    GIT_REPOSITORY  https://github.com/ocornut/imgui
    GIT_TAG         docking
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    SYSTEM          TRUE
)
if(imgui_ADDED)
    add_library(imgui
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    )

    target_include_directories(imgui
        SYSTEM PUBLIC ${imgui_SOURCE_DIR}
        SYSTEM PUBLIC ${imgui_SOURCE_DIR}/backends
        SYSTEM PUBLIC ${glfw_SOURCE_DIR}/include
    )

    if(NOT MSVC)
        target_compile_options(imgui
            PRIVATE -w
        )
    else()
        target_compile_options(imgui
            PRIVATE /W0
        )
    endif()

    target_link_libraries(imgui
            PRIVATE glfw
    )
    if(UNIX AND !APPLE)
        target_link_libraries(imgui
            PRIVATE X11
        )
    endif()
endif()

