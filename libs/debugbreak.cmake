include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            debugbreak
    GIT_REPOSITORY  https://github.com/scottt/debugbreak
    GIT_TAG         v1.0
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    SYSTEM          TRUE
)
if(debugbreak_ADDED)
    add_library(debugbreak
        INTERFACE ${debugbreak_SOURCE_DIR}/debugbreak.h
    )

    target_include_directories(debugbreak
        SYSTEM INTERFACE ${debugbreak_SOURCE_DIR}
    )
endif()
