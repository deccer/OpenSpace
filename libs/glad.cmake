include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            glad
    GIT_REPOSITORY  https://github.com/Dav1dde/glad
    GIT_TAG         v2.0.6
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    SYSTEM          TRUE
)
if(glad_ADDED)
    add_subdirectory("${glad_SOURCE_DIR}/cmake" glad_cmake SYSTEM)
    glad_add_library(glad STATIC REPRODUCIBLE EXCLUDE_FROM_ALL LOADER API gl:core=4.6 EXTENSIONS GL_ARB_bindless_texture GL_EXT_texture_compression_s3tc)
endif()
