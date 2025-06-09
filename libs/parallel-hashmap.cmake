FetchContent_Declare(
    phmap
    GIT_REPOSITORY      https://github.com/greg7mdp/parallel-hashmap.git
    GIT_TAG             v1.3.12
    GIT_SHALLOW         TRUE
    GIT_PROGRESS        TRUE
    UPDATE_DISCONNECTED ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
message("Fetching phmap")
option(PHMAP_BUILD_TESTS "Whether or not to build the tests" OFF)
option(PHMAP_BUILD_EXAMPLES "Whether or not to build the examples" OFF)
FetchContent_MakeAvailable(phmap)
