#include "Engine/Platform/SystemInfo.h"

#include <SDL3/SDL.h>
#include <assimp/version.h>
#include <glad/glad.h>
#include <glm/glm.hpp>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <fstream>
#include <unistd.h>
#endif

namespace
{
    std::string GLString(GLenum name)
    {
        const auto* value = reinterpret_cast<const char*>(glGetString(name));
        return value ? value : "unknown";
    }

    // GL_NVX_gpu_memory_info (NVIDIA) / GL_ATI_meminfo (AMD) values
    constexpr GLenum GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX = 0x9048;
    constexpr GLenum GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX = 0x9049;
    constexpr GLenum TEXTURE_FREE_MEMORY_ATI = 0x87FC;
}

const SystemInfo::Hardware& SystemInfo::GetHardware()
{
    static const Hardware hardware = [] {
        Hardware info;
        info.cpuCores = SDL_GetNumLogicalCPUCores();
        info.cpuCacheLineKB = SDL_GetCPUCacheLineSize();
        info.systemRamMB = SDL_GetSystemRAM();
        const std::pair<bool, const char*> features[] = {
            { SDL_HasSSE(), "SSE" }, { SDL_HasSSE2(), "SSE2" }, { SDL_HasSSE3(), "SSE3" }, { SDL_HasSSE41(), "SSE41" },
            { SDL_HasSSE42(), "SSE42" }, { SDL_HasAVX(), "AVX" }, { SDL_HasAVX2(), "AVX2" }, { SDL_HasAVX512F(), "AVX512F" },
            { SDL_HasMMX(), "MMX" }, { SDL_HasNEON(), "NEON" },
        };
        for (const auto& [present, name] : features)
            if (present)
                info.cpuFeatures += std::string(info.cpuFeatures.empty() ? "" : ", ") + name;
        info.gpuVendor = GLString(GL_VENDOR);
        info.gpuRenderer = GLString(GL_RENDERER);
        info.glVersion = GLString(GL_VERSION);
        info.glslVersion = GLString(GL_SHADING_LANGUAGE_VERSION);
        return info;
    }();
    return hardware;
}

SystemInfo::Versions SystemInfo::GetVersions(int devilVersion)
{
    Versions versions;
    const int sdl = SDL_GetVersion();
    versions.sdl = std::to_string(SDL_VERSIONNUM_MAJOR(sdl)) + "." + std::to_string(SDL_VERSIONNUM_MINOR(sdl)) + "." + std::to_string(SDL_VERSIONNUM_MICRO(sdl));
    versions.opengl = GLString(GL_VERSION);
    versions.devil = std::to_string(devilVersion / 100) + "." + std::to_string((devilVersion / 10) % 10) + "." + std::to_string(devilVersion % 10);
    versions.assimp = std::to_string(aiGetVersionMajor()) + "." + std::to_string(aiGetVersionMinor()) + "." + std::to_string(aiGetVersionPatch());
    versions.glm = std::to_string(GLM_VERSION_MAJOR) + "." + std::to_string(GLM_VERSION_MINOR) + "." + std::to_string(GLM_VERSION_PATCH);
    return versions;
}

std::size_t SystemInfo::GetProcessMemoryBytes()
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters {};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
        return counters.WorkingSetSize;
    return 0;
#elif defined(__linux__)
    std::ifstream statm("/proc/self/statm");
    std::size_t pages = 0, resident = 0;
    if (statm >> pages >> resident)
        return resident * static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    return 0;
#else
    return 0;
#endif
}

SystemInfo::VideoMemory SystemInfo::GetVideoMemory()
{
    VideoMemory memory;
    if (GLAD_GL_NVX_gpu_memory_info)
    {
        GLint total = 0, current = 0;
        glGetIntegerv(GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total);
        glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &current);
        memory = { true, total / 1024, current / 1024 };
    }
    else if (GLAD_GL_ATI_meminfo)
    {
        GLint values[4] = {};
        glGetIntegerv(TEXTURE_FREE_MEMORY_ATI, values);
        memory = { true, 0, values[0] / 1024 };
    }
    return memory;
}
