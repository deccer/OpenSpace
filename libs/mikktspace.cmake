FetchContent_Declare(
    mikktspace
    GIT_REPOSITORY  https://github.com/mmikk/MikkTSpace/
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_GetProperties(mikktspace)
if(NOT mikktspace_POPULATED)
    FetchContent_MakeAvailable(mikktspace)
    message("Fetching mikktspace")

    add_library(mikktspace
        ${mikktspace_SOURCE_DIR}/mikktspace.c
    )
    target_include_directories(mikktspace
        PUBLIC ${mikktspace_SOURCE_DIR}
    )

endif()
