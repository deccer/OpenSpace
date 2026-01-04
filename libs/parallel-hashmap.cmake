include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            phmap
    GIT_REPOSITORY  https://github.com/greg7mdp/parallel-hashmap.git
    GIT_TAG         v1.3.12
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    OPTIONS         "PHMAP_BUILD_TESTS OFF"
    OPTIONS         "PHMAP_BUILD_EXAMPLES OFF"
    SYSTEM          TRUE
)
