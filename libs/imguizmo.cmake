include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            imguizmo
    GIT_REPOSITORY  https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
if(imguizmo_ADDED)
    add_library(imguizmo
        ${imguizmo_SOURCE_DIR}/ImGuizmo.h
        ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
    )

    target_include_directories(imguizmo PUBLIC
        ${imguizmo_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/include)

    target_link_libraries(imguizmo PRIVATE imgui)
endif()

