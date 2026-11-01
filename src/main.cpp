#include "Editor/ModuleEditor.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Log.h"
#include "Engine/Modules/ModuleCamera3D.h"
#include "Engine/Modules/ModuleInput.h"
#include "Engine/Modules/ModuleScene.h"
#include "Engine/Scene/GameObject.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_main.h> // provides WinMain on Windows and calls our main()

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace
{
    // Run from the executable folder so Assets/ is always found (double click, shortcuts...)
    void UseExecutableDirectory()
    {
        if (const char* base = SDL_GetBasePath())
        {
            std::error_code error;
            std::filesystem::current_path(std::filesystem::path(reinterpret_cast<const char8_t*>(base)), error);
        }
    }

    // Optional automatic run used to test the build: --autotest <screenshot.bmp> [--frames N]
    struct AutoTest
    {
        bool enabled = false;
        std::string screenshot;
        unsigned long long frames = 90;
    };
}

int main(int argc, char** argv)
{
    UseExecutableDirectory();

    AutoTest autoTest;
    std::vector<std::string> filesToOpen;
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--autotest" && i + 1 < argc)
        {
            autoTest.enabled = true;
            autoTest.screenshot = argv[++i];
        }
        else if (argument == "--frames" && i + 1 < argc)
            autoTest.frames = std::strtoull(argv[++i], nullptr, 10);
        else if (argument.rfind("--", 0) != 0)
            filesToOpen.push_back(argument); // files passed to the exe behave like a drag & drop
    }

    Application app(argc, argv);
    app.AddModule(std::make_unique<Editor::ModuleEditor>());

    int exitCode = EXIT_SUCCESS;
    if (app.Init())
    {
        for (const std::string& file : filesToOpen)
            app.input->QueueDroppedFile(file);

        LOG("-------------- Application Update --------------");
        UpdateStatus status = UpdateStatus::Continue;
        while (status == UpdateStatus::Continue)
        {
            if (autoTest.enabled && app.GetFrameCount() + 10 == autoTest.frames && !filesToOpen.empty())
                app.camera->Focus(app.scene->GetRoot()->GetHierarchyAABB()); // frame every loaded model
            if (autoTest.enabled && app.GetFrameCount() + 1 == autoTest.frames)
                app.RequestScreenshot(autoTest.screenshot);
            status = app.Update();
            if (autoTest.enabled && app.GetFrameCount() > autoTest.frames)
                app.RequestQuit();
        }
        if (status == UpdateStatus::Error)
            exitCode = EXIT_FAILURE;
    }
    else
    {
        LOG_ERROR("Application failed to initialize");
        exitCode = EXIT_FAILURE;
    }

    if (!app.CleanUp())
        exitCode = EXIT_FAILURE;
    return exitCode;
}
