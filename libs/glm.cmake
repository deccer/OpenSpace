include(../cmake/CPM.cmake)

CPMAddPackage(
    NAME            glm
    GIT_REPOSITORY  https://github.com/g-truc/glm
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)

