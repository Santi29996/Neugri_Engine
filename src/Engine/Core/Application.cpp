#include "Engine/Core/Application.h"

#include "Engine/Core/Log.h"
#include "Engine/Modules/ModuleCamera3D.h"
#include "Engine/Modules/ModuleInput.h"
#include "Engine/Modules/ModuleRenderer3D.h"
#include "Engine/Modules/ModuleResources.h"
#include "Engine/Modules/ModuleScene.h"
#include "Engine/Modules/ModuleWindow.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <algorithm>
#include <vector>

Application* App = nullptr;

Application::Application(int argc, char** argv)
{
    App = this;
    for (int i = 0; i < argc; ++i)
        arguments.emplace_back(argv[i]);

    auto windowModule = std::make_unique<ModuleWindow>();
    auto inputModule = std::make_unique<ModuleInput>();
    auto resourcesModule = std::make_unique<ModuleResources>();
    auto sceneModule = std::make_unique<ModuleScene>();
    auto cameraModule = std::make_unique<ModuleCamera3D>();
    auto rendererModule = std::make_unique<ModuleRenderer3D>();

    window = windowModule.get();
    input = inputModule.get();
    resources = resourcesModule.get();
    scene = sceneModule.get();
    camera = cameraModule.get();
    renderer = rendererModule.get();

    // Order matters: input is read first, the renderer draws last
    AddModule(std::move(windowModule));
    AddModule(std::move(inputModule));
    AddModule(std::move(resourcesModule));
    AddModule(std::move(sceneModule));
    AddModule(std::move(cameraModule));
    AddModule(std::move(rendererModule));
}

Application::~Application()
{
    // Destroy modules in reverse order of creation
    while (!modules.empty())
        modules.pop_back();
    App = nullptr;
}

void Application::AddModule(std::unique_ptr<Module> module)
{
    modules.push_back(std::move(module));
}

bool Application::HasArgument(const std::string& argument) const
{
    return std::find(arguments.begin(), arguments.end(), argument) != arguments.end();
}

bool Application::Init()
{
    LOG("-------------- Application Init --------------");
    for (const auto& module : modules)
    {
        LOG("Init module: %s", module->GetName().c_str());
        if (!module->Init())
        {
            LOG_ERROR("Module %s failed to Init", module->GetName().c_str());
            return false;
        }
    }

    LOG("-------------- Application Start --------------");
    for (const auto& module : modules)
    {
        if (!module->Start())
        {
            LOG_ERROR("Module %s failed to Start", module->GetName().c_str());
            return false;
        }
    }

    lastFrameTicks = SDL_GetPerformanceCounter();
    return true;
}

UpdateStatus Application::Update()
{
    const unsigned long long frameStart = SDL_GetPerformanceCounter();
    dt = static_cast<float>(frameStart - lastFrameTicks) / static_cast<float>(SDL_GetPerformanceFrequency());
    dt = std::min(dt, 0.25f); // avoid huge jumps after a breakpoint or a window drag
    lastFrameTicks = frameStart;

    UpdateStatus status = UpdateStatus::Continue;

    for (auto it = modules.begin(); it != modules.end() && status == UpdateStatus::Continue; ++it)
        status = (*it)->PreUpdate(dt);
    for (auto it = modules.begin(); it != modules.end() && status == UpdateStatus::Continue; ++it)
        status = (*it)->Update(dt);
    for (auto it = modules.begin(); it != modules.end() && status == UpdateStatus::Continue; ++it)
        status = (*it)->PostUpdate(dt);

    if (!screenshotPath.empty())
    {
        SaveScreenshot();
        screenshotPath.clear();
    }

    window->SwapBuffers();
    FinishFrame(frameStart);

    if (quitRequested && status == UpdateStatus::Continue)
        status = UpdateStatus::Stop;
    return status;
}

void Application::FinishFrame(unsigned long long frameStartTicks)
{
    ++frameCount;
    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

    if (fpsCap > 0)
    {
        const double targetMs = 1000.0 / fpsCap;
        const double elapsedMs = (SDL_GetPerformanceCounter() - frameStartTicks) * 1000.0 / frequency;
        if (elapsedMs < targetMs)
            SDL_DelayPrecise(static_cast<Uint64>((targetMs - elapsedMs) * 1000000.0));
    }

    lastFrameMs = static_cast<float>((SDL_GetPerformanceCounter() - frameStartTicks) * 1000.0 / frequency);
    fps = lastFrameMs > 0.0f ? 1000.0f / lastFrameMs : 0.0f;

    std::rotate(fpsHistory.begin(), fpsHistory.begin() + 1, fpsHistory.end());
    std::rotate(msHistory.begin(), msHistory.begin() + 1, msHistory.end());
    fpsHistory.back() = fps;
    msHistory.back() = lastFrameMs;
}

void Application::SaveScreenshot() const
{
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(window->GetWindow(), &width, &height);
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL rows go bottom to top
    std::vector<unsigned char> flipped(pixels.size());
    const std::size_t rowSize = static_cast<std::size_t>(width) * 4;
    for (int y = 0; y < height; ++y)
        std::copy_n(&pixels[(height - 1 - y) * rowSize], rowSize, &flipped[y * rowSize]);

    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_ABGR8888, flipped.data(), static_cast<int>(rowSize));
    if (surface && SDL_SaveBMP(surface, screenshotPath.c_str()))
        LOG("Screenshot saved: %s", screenshotPath.c_str());
    else
        LOG_ERROR("Could not save screenshot %s: %s", screenshotPath.c_str(), SDL_GetError());
    SDL_DestroySurface(surface);
}

bool Application::CleanUp()
{
    LOG("-------------- Application CleanUp --------------");
    bool ok = true;
    for (auto it = modules.rbegin(); it != modules.rend(); ++it)
        ok = (*it)->CleanUp() && ok;
    return ok;
}
