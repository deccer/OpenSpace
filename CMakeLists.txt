cmake_minimum_required(VERSION 3.29.4)

if(EXISTS ".git/")
    find_package(Git REQUIRED)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" describe --tags --match=?[0-9.]*
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE DESCRIBE_NAME COMMAND_ERROR_IS_FATAL ANY
    )
    #set(CPM_DEVELOPMENT "-development-version")
else()
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.git_archival" DESCRIBE_NAME
        REGEX "^describe-name:.*"
    )
endif()
include(cmake/PopulateChangeSetVariables.cmake)

set(OPENSPACE_VERSION_MAJOR 0)
set(OPENSPACE_VERSION_MINOR 1)
set(OPENSPACE_VERSION_PATCH 0)
set(OPENSPACE_VERSION "${OPENSPACE_VERSION_MAJOR}.${OPENSPACE_VERSION_MINOR}.${OPENSPACE_VERSION_PATCH}.${OPENSPACE_REVISION_ID}")
set(OPENSPACE_VERSION_FULL "${OPENSPACE_VERSION_MAJOR}.${OPENSPACE_VERSION_MINOR}.${OPENSPACE_VERSION_PATCH}.${OPENSPACE_REVISION_ID}-${OPENSPACE_SHORT_CHANGE_SET}")
configure_file(${CMAKE_SOURCE_DIR}/src/Version.hpp.in ${CMAKE_SOURCE_DIR}/src/Version.hpp @ONLY)

project(OpenSpace
    VERSION "${EXTRACTED_OPENSPACE_VERSION}"
    DESCRIPTION "A space game written with c++23 and OpenGL 4.6"
    VERSION "${OPENSPACE_VERSION}"
    HOMEPAGE_URL "https://github.com/deccer/OpenSpace"
    LANGUAGES C CXX
)

set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/.cache/cpm" CACHE STRING "CPM cache path")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake")

if(!${MSVC})
    set(CMAKE_CXX_LINKER_LAUNCHER "/usr/bin/ccache")
    set(CMAKE_LINKER_TYPE MOLD)
else()
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.12.25835)
        set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
        set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    #set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-ftime-report")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Xclang -fno-pch-timestamp")
endif()

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

