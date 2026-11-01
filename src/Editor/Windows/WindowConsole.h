#pragma once

#include "Editor/EditorWindow.h"

#include <imgui.h>

namespace Editor
{
    // Shows every LOG of the engine (Assimp, DevIL, library init, application flow, errors)
    class WindowConsole : public EditorWindow
    {
    public:
        WindowConsole() : EditorWindow("Console") {}
        void Draw() override;

    private:
        ImGuiTextFilter filter;
        bool autoScroll = true;
        bool showInfo = true;
        bool showWarnings = true;
        bool showErrors = true;
        std::size_t lastCount = 0;
    };
}
