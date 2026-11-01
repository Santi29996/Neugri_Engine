#include "Editor/Windows/WindowAbout.h"

#include "Engine/Core/Application.h"
#include "Engine/Core/EngineInfo.h"
#include "Engine/Modules/ModuleResources.h"
#include "Engine/Platform/SystemInfo.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include <string>

namespace Editor
{
    namespace
    {
        constexpr const char* MIT_LICENSE =
            "MIT License\n\n"
            "Copyright (c) 2026 Santiago \u00C1lvarez Cervera\n\n"
            "Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated "
            "documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation "
            "the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to "
            "permit persons to whom the Software is furnished to do so, subject to the following conditions:\n\n"
            "The above copyright notice and this permission notice shall be included in all copies or substantial portions of "
            "the Software.\n\n"
            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO "
            "THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
            "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, "
            "TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.";
    }

    void WindowAbout::Draw()
    {
        ImGui::SetNextWindowSize(ImVec2(560, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(GetName().c_str(), OpenFlag()))
        {
            ImGui::End();
            return;
        }

        const std::string title = std::string(EngineInfo::NAME) + " v" + std::string(EngineInfo::VERSION);
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "%s", title.c_str());
        ImGui::TextWrapped("3D game engine made from scratch in C++ for the Game Engines (Motores) subject. "
                           "First delivery: geometry viewer with drag & drop, Unity-like camera and an ImGui editor.");
        const std::string repository(EngineInfo::REPOSITORY_URL);
        if (ImGui::SmallButton(repository.c_str()))
            SDL_OpenURL(repository.c_str());

        ImGui::SeparatorText("Author");
        for (const auto& member : EngineInfo::TEAM)
            ImGui::BulletText("%.*s  (%.*s)", static_cast<int>(member.name.size()), member.name.data(),
                              static_cast<int>(member.github.size()), member.github.data());

        ImGui::SeparatorText("Libraries");
        const SystemInfo::Versions versions = SystemInfo::GetVersions(App->resources->GetDevILVersion());
        ImGui::BulletText("SDL %s", versions.sdl.c_str());
        ImGui::BulletText("OpenGL %s (glad loader)", versions.opengl.c_str());
        ImGui::BulletText("Dear ImGui %s (docking)", IMGUI_VERSION);
        ImGui::BulletText("Assimp %s", versions.assimp.c_str());
        ImGui::BulletText("DevIL %s (+ libpng, zlib)", versions.devil.c_str());
        ImGui::BulletText("glm %s", versions.glm.c_str());

        ImGui::SeparatorText("License");
        ImGui::BeginChild("license", ImVec2(0, 0), ImGuiChildFlags_Borders);
        ImGui::TextWrapped("%s", MIT_LICENSE);
        ImGui::EndChild();
        ImGui::End();
    }
}
