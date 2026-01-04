include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            meshoptimizer
    GIT_REPOSITORY  https://github.com/zeux/meshoptimizer.git
    GIT_TAG         v0.19
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    SYSTEM          TRUE
)
