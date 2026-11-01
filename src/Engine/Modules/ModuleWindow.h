#pragma once

#include "Engine/Core/Module.h"

#include <SDL3/SDL_video.h>

class ModuleWindow : public Module
{
public:
    ModuleWindow();

    bool Init() override;
    bool CleanUp() override;

    void SwapBuffers() const;

    SDL_Window* GetWindow() const { return window; }
    SDL_GLContext GetContext() const { return context; }

    int GetWidth() const;
    int GetHeight() const;
    void SetSize(int width, int height);

    bool IsFullscreen() const { return fullscreen; }
    bool IsResizable() const { return resizable; }
    bool IsBorderless() const { return borderless; }
    bool IsVSync() const { return vsync; }

    void SetFullscreen(bool enabled);
    void SetResizable(bool enabled);
    void SetBorderless(bool enabled);
    void SetVSync(bool enabled);
    void SetTitle(const char* title);

    int GetGLMajor() const { return glMajor; }
    int GetGLMinor() const { return glMinor; }

private:
    bool CreateContext();

    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;

    int startWidth = 1600;
    int startHeight = 900;
    bool fullscreen = false;
    bool resizable = true;
    bool borderless = false;
    bool vsync = true;
    int glMajor = 0;
    int glMinor = 0;
};
