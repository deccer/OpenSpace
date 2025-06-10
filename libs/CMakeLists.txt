include(mimalloc.cmake)
include(debugbreak.cmake)
include(spdlog.cmake)
include(parallel-hashmap.cmake)
include(poolstl.cmake)

if(USE_PROFILER)
    include(tracy.cmake)
endif()

include(glfw.cmake)
include(glad.cmake)
include(glm.cmake)
include(stb.cmake)

include(entt.cmake)

include(imgui.cmake)
include(imguizmo.cmake)

include(ktx.cmake)
include(mikktspace.cmake)
include(fastgltf.cmake)
include(meshoptimizer.cmake)

include(jolt.cmake)

