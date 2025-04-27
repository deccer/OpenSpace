FetchContent_Declare(
    poolSTL
    GIT_REPOSITORY https://github.com/alugowski/poolSTL.git
    GIT_TAG        v0.3.5
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
message("Fetching poolSTL")
FetchContent_MakeAvailable(poolSTL)
