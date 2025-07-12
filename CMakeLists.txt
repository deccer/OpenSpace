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

string(REGEX MATCH "([0-9\\.]+)" EXTRACTED_OPENSPACE_VERSION "${DESCRIBE_NAME}")

project(OpenSpace
    VERSION "${EXTRACTED_OPENSPACE_VERSION}"
    DESCRIPTION "A space game written with c++23 and OpenGL 4.6"
    HOMEPAGE_URL "https://github.com/deccer/OpenSpace"
    LANGUAGES C CXX
)

set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/.cache/cpm" CACHE STRING "CPM cache path")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "")
set(CMAKE_CXX_STANDARD 23)
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
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Xclang -fno-pch-timestamp ${SANITIZER}")
endif()

set(USE_PROFILER OFF CACHE BOOL "Use Profiler")

add_subdirectory(libs)
add_subdirectory(src)
