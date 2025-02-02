include(FetchContent)

#- GLFW-----------------------------------------------------------------------------------------------------------------

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_INCLUDE_NONE ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
message("Fetching glfw")
FetchContent_MakeAvailable(glfw)

#- glad --------------------------------------------------------------------------------------------------------------

if(TARGET glad)
message("already fetched glad")
else()
    FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad
        GIT_TAG        v2.0.6
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )

    FetchContent_GetProperties(glad)
    if(NOT glad_POPULATED)
        message("Fetching glad")
        FetchContent_MakeAvailable(glad)

        add_subdirectory("${glad_SOURCE_DIR}/cmake" glad_cmake)
        glad_add_library(glad REPRODUCIBLE EXCLUDE_FROM_ALL LOADER API gl:core=4.6 EXTENSIONS GL_ARB_bindless_texture GL_EXT_texture_compression_s3tc)
    endif()
endif()

#- glm ----------------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

message("Fetching glm")
FetchContent_MakeAvailable(glm)

#- stb ---------------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    stb
    GIT_REPOSITORY  https://github.com/nothings/stb.git
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_MakeAvailable(stb)
    message("Fetching stb")

    add_library(stb INTERFACE ${stb_SOURCE_DIR}/stb_image.h)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()

#- mikktspace ---------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    mikktspace
    GIT_REPOSITORY  https://github.com/mmikk/MikkTSpace/
    GIT_TAG         master
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)
FetchContent_GetProperties(mikktspace)
if(NOT mikktspace_POPULATED)
    FetchContent_MakeAvailable(mikktspace)
    message("Fetching mikktspace")

    add_library(mikktspace
        ${mikktspace_SOURCE_DIR}/mikktspace.c
    )
    target_include_directories(mikktspace
        PUBLIC ${mikktspace_SOURCE_DIR}
    )

endif()

#- spdlog -------------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.10.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

message("Fetching spdlog")
FetchContent_MakeAvailable(spdlog)

if(USE_PROFILER)
#- tracy --------------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    tracy
    GIT_REPOSITORY  https://github.com/wolfpld/tracy.git
    GIT_TAG         v0.11.1
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
)

set(TRACY_ENABLE ON CACHE BOOL "Enable profiling")
set(TRACY_ONLY_IPV4 ON CACHE BOOL "IPv4 only")
#set(TRACY_NO_SYSTEM_TRACING ON CACHE BOOL "Disable System Tracing")
message("Fetching tracy")
FetchContent_MakeAvailable(tracy)
endif()

#- imgui --------------------------------------------------------------------------------------------------------------

if(TARGET imgui)
message("already fetched imgui")
else()
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    message("Fetching imgui")
    FetchContent_MakeAvailable(imgui)

    add_library(imgui
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)

    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${glfw_SOURCE_DIR}/include)

    target_link_libraries(imgui PRIVATE glfw)
endif()
endif()

#- imguizmo -----------------------------------------------------------------------------------------------------------

if(TARGET imguizmo)
message("already fetched imguizmo")
else()
FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(imguizmo)
if(NOT imguizmo_POPULATED)
    message("Fetching imguizmo")
    FetchContent_MakeAvailable(imguizmo)

    add_library(imguizmo
        ${imguizmo_SOURCE_DIR}/ImGuizmo.h
        ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
    )

    target_include_directories(imguizmo PUBLIC
        ${imguizmo_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/include)

    target_link_libraries(imguizmo PRIVATE imgui)
endif()
endif()

#- fastgltf ------------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG        main
)

message("Fetching fastgltf")
set(FASTGLTF_USE_CUSTOM_SMALLVECTOR OFF CACHE BOOL "Uses a custom SmallVector type optimised for small arrays" FORCE)
set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "Enables test targets for fastgltf" FORCE)
set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "Enables example targets for fastgltf" FORCE)
set(FASTGLTF_ENABLE_DOCS OFF CACHE BOOL "Enables the configuration of targets that build/generate documentation" FORCE)
set(FASTGLTF_ENABLE_GLTF_RS OFF CACHE BOOL "Enables the benchmark usage of gltf-rs" FORCE)
set(FASTGLTF_ENABLE_ASSIMP OFF CACHE BOOL "Enables the benchmark usage of assimp" FORCE)
set(FASTGLTF_ENABLE_DEPRECATED_EXT ON CACHE BOOL "Enables support for deprecated extensions" FORCE)
set(FASTGLTF_DISABLE_CUSTOM_MEMORY_POOLOFF OFF CACHE BOOL "Disables the memory allocation algorithm based on polymorphic resources" FORCE)
set(FASTGLTF_USE_64BIT_FLOAT OFF CACHE BOOL "Default to 64-bit double precision floats for everything" FORCE)
set(FASTGLTF_COMPILE_AS_CPP20 OFF CACHE BOOL "Have the library compile as C++20" FORCE)
set(FASTGLTF_ENABLE_CPP_MODULES OFF CACHE BOOL "Enables the fastgltf::module target, which uses C++20 modules" FORCE)
set(FASTGLTF_USE_STD_MODULE OFF CACHE BOOL "Use the std module when compiling using C++ modules" FORCE)
FetchContent_MakeAvailable(fastgltf)

#- MeshOptimizer -------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    meshoptimizer
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG        v0.19
)
message("Fetching meshoptimizer")
FetchContent_MakeAvailable(meshoptimizer)

#- Ktx -----------------------------------------------------------------------------------------------------------------

# KTX changes global variables CMAKE_ARCHIVE_OUTPUT_DIRECTORY, CMAKE_LIBRARY_OUTPUT_DIRECTORY and
# CMAKE_RUNTIME_OUTPUT_DIRECTORY, save them and restore later
set(HOWTOSCRIPT_TEMPORARY_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(HOWTOSCRIPT_TEMPORARY_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(HOWTOSCRIPT_TEMPORARY_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

FetchContent_Declare(
    ktx
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG        v4.3.2
)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_KTX1 ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_KTX2 ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)

message("Fetching ktx")
FetchContent_MakeAvailable(ktx)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${HOWTOSCRIPT_TEMPORARY_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${HOWTOSCRIPT_TEMPORARY_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${HOWTOSCRIPT_TEMPORARY_RUNTIME_OUTPUT_DIRECTORY})
target_compile_options(ktx
    PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
    -Wno-everything>
    $<$<CXX_COMPILER_ID:MSVC>:
    -w>
)

#- debugbreak --------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    debugbreak
    GIT_REPOSITORY https://github.com/scottt/debugbreak
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(debugbreak)
if(NOT debugbreak_POPULATED)
    message("Fetching debugbreak")
    FetchContent_MakeAvailable(debugbreak)

    add_library(debugbreak INTERFACE ${debugbreak_SOURCE_DIR}/debugbreak.h)
    target_include_directories(debugbreak INTERFACE ${debugbreak_SOURCE_DIR})
endif()

#- entt ---------------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.13.2
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

message("Fetching entt")
FetchContent_MakeAvailable(entt)

#- parallelhashmap -----------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    phmap
    GIT_REPOSITORY      https://github.com/greg7mdp/parallel-hashmap.git
    GIT_TAG             v1.3.12
    UPDATE_DISCONNECTED ON
)

message("Fetching phmap")
option(PHMAP_BUILD_TESTS "Whether or not to build the tests" OFF)
option(PHMAP_BUILD_EXAMPLES "Whether or not to build the examples" OFF)
FetchContent_MakeAvailable(phmap)

#- poolstl ------------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    poolSTL
    GIT_REPOSITORY https://github.com/alugowski/poolSTL.git
    GIT_TAG        v0.3.5
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

message("Fetching poolSTL")
FetchContent_MakeAvailable(poolSTL)

#- mimalloc -----------------------------------------------------------------------------------------------------------

FetchContent_Declare(
    mimalloc
    GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
    GIT_TAG        v2.1.7
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

message("Fetching mimalloc")
FetchContent_MakeAvailable(mimalloc)

#- Jolt Physics -------------------------------------------------------------------------------------------------------

#FetchContent_Declare(
#    JoltPhysics
#    GIT_REPOSITORY  https://github.com/jrouwe/JoltPhysics.git
#    GIT_TAG         master
#    GIT_SHALLOW     TRUE
#    GIT_PROGRESS    TRUE
#)
#FetchContent_GetProperties(JoltPhysics)
#string(TOLOWER "JoltPhysics" lc_JoltPhysics)
#if (NOT ${lc_JoltPhysics}_POPULATED)
#    message("Fetching jolt")
#    FetchContent_MakeAvailable(JoltPhysics)
#    add_subdirectory(${${lc_JoltPhysics}_SOURCE_DIR}/Build ${${lc_JoltPhysics}_BINARY_DIR})
#endif()