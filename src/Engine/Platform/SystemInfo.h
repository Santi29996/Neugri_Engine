#pragma once

#include <cstddef>
#include <string>

// Hardware detection, software versions and memory usage
namespace SystemInfo
{
    struct Hardware
    {
        int cpuCores = 0;
        int cpuCacheLineKB = 0;
        int systemRamMB = 0;
        std::string cpuFeatures;
        std::string gpuVendor;
        std::string gpuRenderer;
        std::string glVersion;
        std::string glslVersion;
    };

    struct Versions
    {
        std::string sdl;
        std::string opengl;
        std::string devil;
        std::string assimp;
        std::string glm;
    };

    struct VideoMemory
    {
        bool available = false;
        int totalMB = 0;
        int freeMB = 0;
    };

    const Hardware& GetHardware(); // queried once (needs the OpenGL context)
    Versions GetVersions(int devilVersion);
    std::size_t GetProcessMemoryBytes();
    VideoMemory GetVideoMemory();
}
