#pragma once

#include "Engine/Core/Module.h"

#include <memory>
#include <vector>

namespace Editor
{
    class EditorWindow;
    class WindowScene;

    // Editor layer: ImGui initialization, dockspace, main menu bar and all panels.
    // The engine knows nothing about this module: it is added from main().
    class ModuleEditor : public Module
    {
    public:
        ModuleEditor();
        ~ModuleEditor() override;

        bool Init() override;
        UpdateStatus PostUpdate(float dt) override;
        bool CleanUp() override;

    private:
        void DrawMainMenuBar();
        void DrawDockspace();
        void DrawAssetsMenu();
        void HandleShortcuts();
        EditorWindow* FindWindow(const char* name) const;

        template<typename T>
        T* AddWindow()
        {
            auto window = std::make_unique<T>();
            T* raw = window.get();
            windows.push_back(std::move(window));
            return raw;
        }

        std::vector<std::unique_ptr<EditorWindow>> windows;
        bool showDemoWindow = false;
        bool layoutInitialized = false;
        bool layoutResetRequested = false;
        bool quitRequested = false;
    };
}
