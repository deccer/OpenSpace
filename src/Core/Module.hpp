#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using TModuleHandle = HMODULE;
#define LoadModule(name) LoadLibraryA(name.data())
#define GetModuleSymbol GetProcAddress
#define UnloadModule FreeLibrary

#else

#include <dlfcn.h>

using TModuleHandle = void*;
#define LoadModule(name) dlopen(name.data(), RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND)
#define GetModuleSymbol dlsym
#define UnloadModule dlclose

#endif
