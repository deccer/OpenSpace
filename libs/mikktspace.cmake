include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            mikktspace
    GIT_REPOSITORY  https://github.com/mmikk/MikkTSpace.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    SYSTEM          TRUE
)
if(mikktspace_ADDED)
    add_library(mikktspace
        ${mikktspace_SOURCE_DIR}/mikktspace.c
    )
    target_include_directories(mikktspace
        SYSTEM PUBLIC ${mikktspace_SOURCE_DIR}
    )

    if(NOT MSVC)
        target_compile_options(mikktspace
            PRIVATE -w)
    else()
        target_compile_options(mikktspace
            PRIVATE /W0
        )
    endif()
endif()
