cmake_minimum_required(VERSION 3.30)

cmake_policy(SET CMP0091 NEW)  # MSVC runtime library flags selected by CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0092 NEW)  # MSVC warning flags are not in CMAKE_<LANG>_FLAGS by default
cmake_policy(SET CMP0135 NEW)  # ExternalProject download methods timestamp

set(CMAKE_POLICY_VERSION_MINIMUM "3.10")
set(CMAKE_WARN_DEPRECATED FALSE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CLANGD_COMPILATIONDB_DIR "${CMAKE_BINARY_DIR}")
configure_file(
    ${CMAKE_SOURCE_DIR}/.clangd.in
    ${CMAKE_SOURCE_DIR}/.clangd
    @ONLY
)

include(cmake/PopulateChangeSetVariables.cmake)

set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/.cache/cpm" CACHE STRING "CPM cache path")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake")

set(OPENSPACE_VERSION_MAJOR 0)
set(OPENSPACE_VERSION_MINOR 1)
set(OPENSPACE_VERSION_PATCH 0)
set(OPENSPACE_VERSION "${OPENSPACE_VERSION_MAJOR}.${OPENSPACE_VERSION_MINOR}.${OPENSPACE_VERSION_PATCH}.${OPENSPACE_REVISION_ID}")
set(OPENSPACE_VERSION_FULL "${OPENSPACE_VERSION_MAJOR}.${OPENSPACE_VERSION_MINOR}.${OPENSPACE_VERSION_PATCH}.${OPENSPACE_REVISION_ID}-${OPENSPACE_SHORT_CHANGE_SET}")
configure_file(${CMAKE_SOURCE_DIR}/src/Version.hpp.in ${CMAKE_SOURCE_DIR}/src/Version.hpp @ONLY)

project(OpenSpace
    VERSION "${OPENSPACE_VERSION}"
    DESCRIPTION "A space game written in C++23 and powered by OpenGL 4.6"
    HOMEPAGE_URL "https://github.com/deccer/OpenSpace"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT MSVC)
    set(CMAKE_CXX_LINKER_LAUNCHER "/usr/bin/ccache")
    set(CMAKE_LINKER_TYPE MOLD)
else ()
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    #set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-ftime-report")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Xclang -fno-pch-timestamp")
endif ()

set(USE_PROFILER OFF CACHE BOOL "Use Profiler")

add_subdirectory(libs)
add_subdirectory(src)

message(STATUS "=======================================================")
message(STATUS "GIT Revision ID: ${OPENSPACE_REVISION_ID}")
message(STATUS "GIT Short ChangeSet: ${OPENSPACE_SHORT_CHANGE_SET}")
message(STATUS "GIT ChangeSet: ${OPENSPACE_CHANGE_SET}")
message(STATUS "GIT ChangeSet Date: ${OPENSPACE_CHANGE_SET_DATE}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "=======================================================")

