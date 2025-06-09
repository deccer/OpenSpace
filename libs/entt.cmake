FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.15.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
)
message("Fetching entt")
FetchContent_MakeAvailable(entt)
