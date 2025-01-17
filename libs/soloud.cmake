FetchContent_Declare(
    soloud
    GIT_REPOSITORY https://github.com/jarikomppa/soloud.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(soloud)
if(NOT soloud_POPULATED)
    message("Fetching soloud")
    FetchContent_MakeAvailable(soloud)

    file(GLOB_RECURSE SOLOUD_AUDIOSOURCE_SRC_FILES ${soloud_SOURCE_DIR}/src/audiosource/*.cpp ${soloud_SOURCE_DIR}/src/audiosource/*.c)
    file(GLOB SOLOUD_BACKEND_SRC_FILES ${soloud_SOURCE_DIR}/src/backend/sdl2_static/*.cpp)

    file(GLOB SOLOUD_CORE_SRC_FILES ${soloud_SOURCE_DIR}/src/core/*.cpp)
    file(GLOB SOLOUD_FILTER_SRC_FILES ${soloud_SOURCE_DIR}/src/filter/*.cpp)

    add_library(soloud
        ${SOLOUD_AUDIOSOURCE_SRC_FILES}
        ${SOLOUD_BACKEND_SRC_FILES}
        ${SOLOUD_CORE_SRC_FILES}
        ${SOLOUD_FILTER_SRC_FILES}
    )

    target_include_directories(soloud
        PUBLIC ${soloud_SOURCE_DIR}/include
        PUBLIC ${sdl2_SOURCE_DIR}/include
    )

    target_compile_definitions(soloud
        #PRIVATE WITH_NULL
        #PRIVATE WITH_NOSOUND
        #PRIVATE WITH_SDL2
        PRIVATE WITH_SDL2_STATIC
        #PRIVATE WITH_SDL1
        #PRIVATE WITH_SDL1_STATIC
        #PRIVATE WITH_PORTAUDIO
        #PRIVATE WITH_OPENAL
        #PRIVATE WITH_XAUDIO2
        #PRIVATE WITH_WINMM
        #PRIVATE WITH_WASAPI
        #PRIVATE WITH_OSS
        #PRIVATE WITH_ALSA
        #PRIVATE WITH_OPENSLES
        #PRIVATE WITH_COREAUDIO
        #PRIVATE WITH_VITA_HOMEBREW
        #PRIVATE WITH_JACK
        #PRIVATE WITH_MINIAUDIO
    )

    target_link_libraries(soloud
        PUBLIC SDL2::SDL2-static
    )
endif()
