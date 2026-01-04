include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            spdlog
    GIT_REPOSITORY  https://github.com/gabime/spdlog.git
    GIT_TAG         v1.15.2
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    SYSTEM          TRUE
)
