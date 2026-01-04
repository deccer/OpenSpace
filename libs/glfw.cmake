include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            glfw
    GIT_REPOSITORY  https://github.com/glfw/glfw
    GIT_TAG         3.4
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    OPTIONS         "BUILD_SHARED_LIBS OFF"
    OPTIONS         "GLFW_BUILD_EXAMPLES OFF"
    OPTIONS         "GLFW_BUILD_TESTS OFF"
    OPTIONS         "GLFW_BUILD_DOCS OFF"
    OPTIONS         "GLFW_VULKAN_STATIC OFF"
    OPTIONS         "GLFW_INSTALL OFF"
    OPTIONS         "GLFW_INCLUDE_NONE ON"
    SYSTEM          TRUE
)
