
add_library(Assets SHARED
    AssetStorage.cpp
)

add_dependencies(Assets
    Logger
)

target_compile_definitions(Assets
    PRIVATE GLM_FORCE_CXX2A
    PRIVATE GLM_FORCE_RADIANS
    PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE
    PRIVATE GLM_FORCE_RIGHT_HANDED
    PRIVATE GLM_FORCE_SILENT_WARNINGS
    PRIVATE GLM_FORCE_SWIZZLE
    PRIVATE GLM_FORCE_XYZW_ONLY
    PRIVATE GLM_ENABLE_EXPERIMENTAL
)

target_link_libraries(Assets
    PUBLIC Logger
    PUBLIC phmap
    PUBLIC glm
    PRIVATE fastgltf
)
