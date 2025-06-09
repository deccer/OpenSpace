FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG        main
    EXCLUDE_FROM_ALL
    SYSTEM
)
message("Fetching fastgltf")
set(FASTGLTF_USE_CUSTOM_SMALLVECTOR OFF CACHE BOOL "Uses a custom SmallVector type optimised for small arrays" FORCE)
set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "Enables test targets for fastgltf" FORCE)
set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "Enables example targets for fastgltf" FORCE)
set(FASTGLTF_ENABLE_DOCS OFF CACHE BOOL "Enables the configuration of targets that build/generate documentation" FORCE)
set(FASTGLTF_ENABLE_GLTF_RS OFF CACHE BOOL "Enables the benchmark usage of gltf-rs" FORCE)
set(FASTGLTF_ENABLE_ASSIMP OFF CACHE BOOL "Enables the benchmark usage of assimp" FORCE)
set(FASTGLTF_ENABLE_DEPRECATED_EXT ON CACHE BOOL "Enables support for deprecated extensions" FORCE)
set(FASTGLTF_DISABLE_CUSTOM_MEMORY_POOLOFF OFF CACHE BOOL "Disables the memory allocation algorithm based on polymorphic resources" FORCE)
set(FASTGLTF_USE_64BIT_FLOAT OFF CACHE BOOL "Default to 64-bit double precision floats for everything" FORCE)
set(FASTGLTF_COMPILE_AS_CPP20 OFF CACHE BOOL "Have the library compile as C++20" FORCE)
set(FASTGLTF_ENABLE_CPP_MODULES OFF CACHE BOOL "Enables the fastgltf::module target, which uses C++20 modules" FORCE)
set(FASTGLTF_USE_STD_MODULE OFF CACHE BOOL "Use the std module when compiling using C++ modules" FORCE)
FetchContent_MakeAvailable(fastgltf)
