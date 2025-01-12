FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG        main
)

message("Fetching fastgltf")
FetchContent_MakeAvailable(fastgltf)
