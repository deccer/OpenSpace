FetchContent_Declare(
    mikktspace
    GIT_REPOSITORY  https://github.com/mmikk/MikkTSpace/
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_GetProperties(mikktspace)
if(NOT mikktspace_POPULATED)
    message("Fetching mikktspace")
    FetchContent_MakeAvailable(mikktspace)

    add_library(mikktspace
        ${mikktspace_SOURCE_DIR}/mikktspace.c
    )
    target_include_directories(mikktspace
        PUBLIC ${mikktspace_SOURCE_DIR}
    )
endif()