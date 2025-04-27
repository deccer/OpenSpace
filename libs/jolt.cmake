FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY  https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_GetProperties(JoltPhysics)
string(TOLOWER "JoltPhysics" lc_JoltPhysics)
if (NOT ${lc_JoltPhysics}_POPULATED)
    message("Fetching jolt")
    FetchContent_MakeAvailable(JoltPhysics)
    add_subdirectory(${${lc_JoltPhysics}_SOURCE_DIR}/Build ${${lc_JoltPhysics}_BINARY_DIR})
endif()
