cmake_minimum_required(VERSION 3.29.4)

project(OpenSpace
    VERSION 1.0.0
    DESCRIPTION "OpenSpace Game"
    HOMEPAGE_URL https://github.com/deccer/OpenSpace
)

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
if(!${MSVC})
    set(CMAKE_CXX_LINKER_LAUNCHER "/usr/bin/ccache")
    set(CMAKE_LINKER_TYPE MOLD)
    #set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-ftime-report")
else()
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.12.25835)
        set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
        set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
    endif()
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(OPENSPACE_USE_PROFILER OFF CACHE BOOL "Use Profiler")
set(OPENSPACE_HOTRELOAD_GAME_ENABLED OFF CACHE BOOL "Enable Game library hot reload")

add_subdirectory(libs)
add_subdirectory(src)
