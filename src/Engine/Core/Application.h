#pragma once

#include "Engine/Core/Module.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

class ModuleWindow;
class ModuleInput;
class ModuleResources;
class ModuleScene;
class ModuleCamera3D;
class ModuleRenderer3D;

// Owns all modules and runs the main loop. The editor is added from main() as an
// extra module so the engine code never depends on it.
class Application
{
public:
    Application(int argc, char** argv);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void AddModule(std::unique_ptr<Module> module);

    bool Init();
    UpdateStatus Update();
    bool CleanUp();

    void RequestQuit() { quitRequested = true; }
    void RequestScreenshot(std::string path) { screenshotPath = std::move(path); }

    // Frame timing
    static constexpr std::size_t FPS_HISTORY = 100;
    float GetFPS() const { return fps; }
    float GetLastFrameMs() const { return lastFrameMs; }
    const std::array<float, FPS_HISTORY>& GetFPSHistory() const { return fpsHistory; }
    const std::array<float, FPS_HISTORY>& GetMsHistory() const { return msHistory; }
    int GetFPSCap() const { return fpsCap; }
    void SetFPSCap(int cap) { fpsCap = cap < 0 ? 0 : cap; }
    unsigned long long GetFrameCount() const { return frameCount; }

    const std::vector<std::string>& GetArguments() const { return arguments; }
    bool HasArgument(const std::string& argument) const;

    // Direct access to the engine modules (owned by `modules`)
    ModuleWindow* window = nullptr;
    ModuleInput* input = nullptr;
    ModuleResources* resources = nullptr;
    ModuleScene* scene = nullptr;
    ModuleCamera3D* camera = nullptr;
    ModuleRenderer3D* renderer = nullptr;

private:
    void FinishFrame(unsigned long long frameStartTicks);
    void SaveScreenshot() const;

    std::vector<std::unique_ptr<Module>> modules;
    std::vector<std::string> arguments;

    bool quitRequested = false;
    std::string screenshotPath;

    float dt = 0.0f;
    float fps = 0.0f;
    float lastFrameMs = 0.0f;
    int fpsCap = 0; // 0 = unlimited (vsync still applies)
    unsigned long long frameCount = 0;
    unsigned long long lastFrameTicks = 0;
    std::array<float, FPS_HISTORY> fpsHistory {};
    std::array<float, FPS_HISTORY> msHistory {};
};

extern Application* App;
