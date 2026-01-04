include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            mimalloc
    GIT_REPOSITORY  https://github.com/microsoft/mimalloc.git
    GIT_TAG         v2.1.7
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    SYSTEM          TRUE
)
