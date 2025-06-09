FetchContent_Declare(
    meshoptimizer
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG        v0.19
    EXCLUDE_FROM_ALL
    SYSTEM
)
message("Fetching meshoptimizer")
FetchContent_MakeAvailable(meshoptimizer)
