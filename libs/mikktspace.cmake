include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            mikktspace
    GIT_REPOSITORY  https://github.com/mmikk/MikkTSpace.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
if(mikktspace_ADDED)
    add_library(mikktspace
        ${mikktspace_SOURCE_DIR}/mikktspace.c
    )
    target_include_directories(mikktspace
        PUBLIC ${mikktspace_SOURCE_DIR}
    )
endif()
