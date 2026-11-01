#pragma once

#include "Editor/EditorWindow.h"

#include <array>

namespace Editor
{
    // FPS graph, every module setting, memory consumption, hardware and software versions
    class WindowConfiguration : public EditorWindow
    {
    public:
        WindowConfiguration() : EditorWindow("Configuration") {}
        void Draw() override;

    private:
        void DrawApplication();
        void DrawWindow();
        void DrawRenderer();
        void DrawCamera();
        void DrawInput();
        void DrawTextures();
        void DrawMemory();
        void DrawHardware();

        std::array<float, 100> memoryHistory {};
        int windowWidth = 0;
        int windowHeight = 0;
    };
}
