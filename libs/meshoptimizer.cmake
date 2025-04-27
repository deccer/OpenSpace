FetchContent_Declare(
    meshoptimizer
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG        v0.19
)
message("Fetching meshoptimizer")
FetchContent_MakeAvailable(meshoptimizer)
