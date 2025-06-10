include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            stb
    GIT_REPOSITORY  https://github.com/nothings/stb.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
if(stb_ADDED)
    add_library(stb INTERFACE ${stb_SOURCE_DIR}/stb_image.h)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()
