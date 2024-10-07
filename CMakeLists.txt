cmake_minimum_required(VERSION 3.29.4)

project(HowToScript
    VERSION 1.0.0
)

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "")
set(CMAKE_CXX_STANDARD 23)
if(!${MSVC})
    set(CMAKE_CXX_LINKER_LAUNCHER "/usr/bin/ccache")
    set(CMAKE_LINKER_TYPE MOLD)
    #set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-ftime-report")
endif()

add_subdirectory(libs)
add_subdirectory(src)