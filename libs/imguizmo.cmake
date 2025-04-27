if(TARGET imguizmo)
    message("already fetched imguizmo")
else()
    FetchContent_Declare(
        imguizmo
        GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_GetProperties(imguizmo)
    if(NOT imguizmo_POPULATED)
        message("Fetching imguizmo")
        FetchContent_MakeAvailable(imguizmo)

        add_library(imguizmo
            ${imguizmo_SOURCE_DIR}/ImGuizmo.h
            ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
        )

        target_include_directories(imguizmo PUBLIC
            ${imguizmo_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/include)

        target_link_libraries(imguizmo PRIVATE imgui)
    endif()
endif()
