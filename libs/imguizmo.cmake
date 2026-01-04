include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            imguizmo
    GIT_REPOSITORY  https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    SYSTEM          TRUE
)
if(imguizmo_ADDED)
    add_library(imguizmo
        ${imguizmo_SOURCE_DIR}/ImGuizmo.h
        ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
    )

    target_include_directories(imguizmo
        SYSTEM PUBLIC ${imguizmo_SOURCE_DIR}
        SYSTEM PUBLIC ${imgui_SOURCE_DIR}/include
    )

    if(NOT MSVC)
        target_compile_options(imguizmo
            PRIVATE -w
        )
    else()
        target_compile_options(imguizmo
            PRIVATE /W0
        )
    endif()

    target_link_libraries(imguizmo PRIVATE imgui)
endif()

