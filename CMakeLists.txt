cmake_minimum_required(VERSION 3.22)

project(HowToScript
    VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "")

add_subdirectory(libs)
add_subdirectory(src)