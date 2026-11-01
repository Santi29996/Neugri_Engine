#pragma once

#include "Editor/EditorWindow.h"

namespace Editor
{
    // Shows the framebuffer the renderer draws the scene into
    class WindowScene : public EditorWindow
    {
    public:
        WindowScene() : EditorWindow("Scene") {}
        void Draw() override;

    private:
        bool showHelp = true;
    };
}
