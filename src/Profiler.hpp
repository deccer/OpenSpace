#pragma once

#ifdef USE_PROFILER
#include <glad/gl.h>
#include <tracy/TracyOpenGL.hpp>
#define PROFILER_GPUCONTEXT TracyGpuContext
#define PROFILER_GPUCOLLECT TracyGpuCollect
#define PROFILER_FRAMEMARK FrameMark
#define PROFILER_ZONESCOPEDN(x) ZoneScopedN(x)
#else
#define PROFILER_GPUCONTEXT
#define PROFILER_GPUCOLLECT
#define PROFILER_FRAMEMARK
#define PROFILER_ZONESCOPEDN(x)
#endif
