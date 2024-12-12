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
#if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#  # Workaround for std::expected not available in clang
#  add_compile_options(
#    -stdlib=libstdc++ -D__cpp_concepts=202002 -Wno-builtin-macro-redefined
#  )
#endif()

set(USE_PROFILER OFF CACHE BOOL "Use Profiler")

add_subdirectory(libs)
add_subdirectory(src)