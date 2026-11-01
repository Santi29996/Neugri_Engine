#pragma once

#include "Editor/EditorWindow.h"

namespace Editor
{
    // Engine name + version + team + libraries + license
    class WindowAbout : public EditorWindow
    {
    public:
        WindowAbout() : EditorWindow("About") {}
        void Draw() override;
    };
}
