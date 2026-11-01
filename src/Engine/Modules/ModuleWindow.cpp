#include "Engine/Modules/ModuleWindow.h"

#include "Engine/Core/EngineInfo.h"
#include "Engine/Core/Log.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <string>

ModuleWindow::ModuleWindow() : Module("Window") {}

bool ModuleWindow::Init()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        LOG_ERROR("SDL could not be initialized: %s", SDL_GetError());
        return false;
    }
    const int sdlVersion = SDL_GetVersion();
    LOG("SDL %d.%d.%d initialized", SDL_VERSIONNUM_MAJOR(sdlVersion), SDL_VERSIONNUM_MINOR(sdlVersion), SDL_VERSIONNUM_MICRO(sdlVersion));

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    if (borderless) flags |= SDL_WINDOW_BORDERLESS;

    const std::string title = std::string(EngineInfo::NAME) + " v" + std::string(EngineInfo::VERSION);
    window = SDL_CreateWindow(title.c_str(), startWidth, startHeight, flags | SDL_WINDOW_HIDDEN);
    if (!window)
    {
        LOG_ERROR("Window could not be created: %s", SDL_GetError());
        return false;
    }

    if (!CreateContext())
        return false;

    SetVSync(vsync);
    SDL_ShowWindow(window);
    return true;
}

bool ModuleWindow::CreateContext()
{
    // Try the newest version first and fall back for older drivers
    constexpr int versions[][2] = { { 4, 6 }, { 4, 5 }, { 4, 3 }, { 3, 3 } };
    for (const auto& version : versions)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, version[0]);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, version[1]);
        context = SDL_GL_CreateContext(window);
        if (context)
            break;
        LOG_WARNING("OpenGL %d.%d context not available: %s", version[0], version[1], SDL_GetError());
    }
    if (!context)
    {
        LOG_ERROR("OpenGL context could not be created: %s", SDL_GetError());
        return false;
    }
    SDL_GL_MakeCurrent(window, context);

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)) == 0)
    {
        LOG_ERROR("Error loading the glad library");
        return false;
    }
    glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
    glGetIntegerv(GL_MINOR_VERSION, &glMinor);

    LOG("glad initialized: OpenGL %d.%d core profile", glMajor, glMinor);
    LOG("Vendor: %s", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    LOG("Renderer: %s", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    LOG("OpenGL version supported %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    LOG("GLSL: %s", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    return true;
}

bool ModuleWindow::CleanUp()
{
    LOG("Destroying OpenGL context and SDL window");
    if (context)
        SDL_GL_DestroyContext(context);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
    context = nullptr;
    window = nullptr;
    return true;
}

void ModuleWindow::SwapBuffers() const
{
    SDL_GL_SwapWindow(window);
}

int ModuleWindow::GetWidth() const
{
    int width = 0;
    SDL_GetWindowSize(window, &width, nullptr);
    return width;
}

int ModuleWindow::GetHeight() const
{
    int height = 0;
    SDL_GetWindowSize(window, nullptr, &height);
    return height;
}

void ModuleWindow::SetSize(int width, int height)
{
    SDL_SetWindowSize(window, width, height);
}

void ModuleWindow::SetFullscreen(bool enabled)
{
    if (SDL_SetWindowFullscreen(window, enabled))
        fullscreen = enabled;
    else
        LOG_ERROR("Could not change fullscreen mode: %s", SDL_GetError());
}

void ModuleWindow::SetResizable(bool enabled)
{
    if (SDL_SetWindowResizable(window, enabled))
        resizable = enabled;
}

void ModuleWindow::SetBorderless(bool enabled)
{
    if (SDL_SetWindowBordered(window, !enabled))
        borderless = enabled;
}

void ModuleWindow::SetVSync(bool enabled)
{
    if (SDL_GL_SetSwapInterval(enabled ? 1 : 0))
        vsync = enabled;
    else
        LOG_WARNING("Could not change VSync: %s", SDL_GetError());
}

void ModuleWindow::SetTitle(const char* title)
{
    SDL_SetWindowTitle(window, title);
}
