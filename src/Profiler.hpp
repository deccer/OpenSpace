#pragma once

#ifdef USE_PROFILER
#include <glad/gl.h>
#include <tracy/TracyOpenGL.hpp>
#define PROFILER_ZONESCOPEDN(x) ZoneScopedN(x)
#else
#define PROFILER_ZONESCOPEDN(x)
#endif
