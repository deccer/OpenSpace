include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            tracy
    GIT_REPOSITORY  https://github.com/wolfpld/tracy.git
    GIT_TAG         v0.11.1
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    OPTIONS         "TRACY_ENABLE ON"
    OPTIONS         "TRACY_ONLY_IPV4 ON"
    #OPTIONS         "TRACY_NO_SYSTEM_TRACING ON"
    SYSTEM          TRUE
)
