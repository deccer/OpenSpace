add_custom_target(CopyData ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data
)

# ---- LOGGER ----

add_library(Logger SHARED
    Logger.cpp
)

# ---- GAME ----

add_library(Game SHARED
    Game.cpp
)

add_dependencies(Game
    Logger
)

set_target_properties(Game PROPERTIES PREFIX "")

add_custom_command(TARGET Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_CURRENT_BINARY_DIR}/libGame.so
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Game> ${CMAKE_CURRENT_BINARY_DIR}/libGame.so
    COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_FILE:Game>
)

target_compile_definitions(Game
    PRIVATE GLM_FORCE_CXX2A
    PRIVATE GLM_FORCE_RADIANS
    PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE
    PRIVATE GLM_FORCE_RIGHT_HANDED
    PRIVATE GLM_FORCE_SILENT_WARNINGS
    PRIVATE GLM_FORCE_SWIZZLE
    PRIVATE GLM_FORCE_XYZW_ONLY
    PRIVATE GLM_ENABLE_EXPERIMENTAL
)

target_link_libraries(Game
    PRIVATE Logger
)

# --- GAMEHOST ---

add_executable(GameHost
    Io.cpp
    Images.cpp
    GameHost.cpp
    Main.cpp
)
add_dependencies(GameHost
    CopyData
    Logger
    Game
)

target_compile_definitions(GameHost
    PRIVATE GLM_FORCE_CXX2A
    PRIVATE GLM_FORCE_RADIANS
    PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE
    PRIVATE GLM_FORCE_RIGHT_HANDED
    PRIVATE GLM_FORCE_SILENT_WARNINGS
    PRIVATE GLM_FORCE_SWIZZLE
    PRIVATE GLM_FORCE_XYZW_ONLY
    PRIVATE GLM_ENABLE_EXPERIMENTAL
)

target_link_libraries(GameHost
    PRIVATE glfw
    PRIVATE glad
    PRIVATE glm
    PRIVATE imgui
    PRIVATE imguizmo
    PRIVATE stb
    PRIVATE debugbreak
    PRIVATE poolSTL::poolSTL
    #PRIVATE tbb
    PRIVATE fastgltf
    PRIVATE EnTT
    #PRIVATE mimalloc-static
    #PRIVATE Jolt
    PRIVATE mikktspace

    PRIVATE Logger
)

if(UNIX AND NOT APPLE)
    target_link_libraries(GameHost
        PRIVATE dl
    )
endif()

if(USE_PROFILER)
    target_compile_definitions(GameHost
        PRIVATE USE_PROFILER
    )
    target_link_libraries(GameHost
        PRIVATE TracyClient
    )
endif()
