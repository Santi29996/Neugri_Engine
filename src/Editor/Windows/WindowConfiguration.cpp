#include "Editor/Windows/WindowConfiguration.h"

#include "Engine/Core/Application.h"
#include "Engine/Core/EngineInfo.h"
#include "Engine/Importers/ModelImporter.h"
#include "Engine/Modules/ModuleCamera3D.h"
#include "Engine/Modules/ModuleInput.h"
#include "Engine/Modules/ModuleRenderer3D.h"
#include "Engine/Modules/ModuleResources.h"
#include "Engine/Modules/ModuleScene.h"
#include "Engine/Modules/ModuleWindow.h"
#include "Engine/Platform/SystemInfo.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <string>

namespace Editor
{
    namespace
    {
        constexpr ImVec4 VALUE_COLOR(1.0f, 0.75f, 0.2f, 1.0f);

        void Value(const char* label, const std::string& value)
        {
            ImGui::TextUnformatted(label);
            ImGui::SameLine();
            ImGui::TextColored(VALUE_COLOR, "%s", value.c_str());
        }

        std::string Megabytes(std::size_t bytes)
        {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%.2f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
            return buffer;
        }
    }

    void WindowConfiguration::Draw()
    {
        // Sample the process memory every frame the panel is drawn
        std::rotate(memoryHistory.begin(), memoryHistory.begin() + 1, memoryHistory.end());
        memoryHistory.back() = static_cast<float>(SystemInfo::GetProcessMemoryBytes()) / (1024.0f * 1024.0f);

        if (!ImGui::Begin(GetName().c_str(), OpenFlag()))
        {
            ImGui::End();
            return;
        }
        DrawApplication();
        DrawWindow();
        DrawRenderer();
        DrawCamera();
        DrawInput();
        DrawTextures();
        DrawMemory();
        DrawHardware();
        ImGui::End();
    }

    void WindowConfiguration::DrawApplication()
    {
        if (!ImGui::CollapsingHeader("Application", ImGuiTreeNodeFlags_DefaultOpen))
            return;
        Value("Engine:", std::string(EngineInfo::NAME) + " v" + std::string(EngineInfo::VERSION));

        int cap = App->GetFPSCap();
        if (ImGui::SliderInt("Max FPS", &cap, 0, 240, cap == 0 ? "Unlimited" : "%d"))
            App->SetFPSCap(cap);
        Value("Frames:", std::to_string(App->GetFrameCount()));

        char title[64];
        const auto& fps = App->GetFPSHistory();
        std::snprintf(title, sizeof(title), "Framerate %.1f", fps.back());
        ImGui::PlotHistogram("##framerate", fps.data(), static_cast<int>(fps.size()), 0, title, 0.0f, 200.0f, ImVec2(0, 90));
        const auto& ms = App->GetMsHistory();
        std::snprintf(title, sizeof(title), "Milliseconds %.2f", ms.back());
        ImGui::PlotHistogram("##milliseconds", ms.data(), static_cast<int>(ms.size()), 0, title, 0.0f, 40.0f, ImVec2(0, 90));
    }

    void WindowConfiguration::DrawWindow()
    {
        if (!ImGui::CollapsingHeader("Window"))
            return;
        ModuleWindow* window = App->window;
        if (!ImGui::IsAnyItemActive())
        {
            windowWidth = window->GetWidth();
            windowHeight = window->GetHeight();
        }
        if (ImGui::SliderInt("Width", &windowWidth, 640, 3840))
            window->SetSize(windowWidth, windowHeight);
        if (ImGui::SliderInt("Height", &windowHeight, 360, 2160))
            window->SetSize(windowWidth, windowHeight);

        bool fullscreen = window->IsFullscreen();
        if (ImGui::Checkbox("Fullscreen", &fullscreen))
            window->SetFullscreen(fullscreen);
        ImGui::SameLine();
        bool resizable = window->IsResizable();
        if (ImGui::Checkbox("Resizable", &resizable))
            window->SetResizable(resizable);
        bool borderless = window->IsBorderless();
        if (ImGui::Checkbox("Borderless", &borderless))
            window->SetBorderless(borderless);
        ImGui::SameLine();
        bool vsync = window->IsVSync();
        if (ImGui::Checkbox("VSync", &vsync))
            window->SetVSync(vsync);
    }

    void WindowConfiguration::DrawRenderer()
    {
        if (!ImGui::CollapsingHeader("Renderer"))
            return;
        RenderSettings& settings = App->renderer->settings;
        ImGui::Checkbox("Depth test", &settings.depthTest);
        ImGui::SameLine(160);
        ImGui::Checkbox("Cull face", &settings.cullFace);
        ImGui::Checkbox("Lighting", &settings.lighting);
        ImGui::SameLine(160);
        ImGui::Checkbox("Textures", &settings.textures);
        ImGui::Checkbox("Wireframe", &settings.wireframe);
        ImGui::SameLine(160);
        ImGui::Checkbox("Grid", &settings.showGrid);
        ImGui::Checkbox("Selection AABB", &settings.showSelectionAABB);
        ImGui::SameLine(160);
        ImGui::Checkbox("All AABBs", &settings.showAllAABB);
        ImGui::Checkbox("Checkers on every mesh", &settings.checkersEverywhere);
        ImGui::SliderFloat("Normals length", &settings.normalsLength, 0.01f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::ColorEdit3("Background", &settings.backgroundColor.x);
        ImGui::SliderFloat3("Light direction", &settings.lightDirection.x, -1.0f, 1.0f);

        const RenderStats& stats = App->renderer->GetStats();
        Value("Draw calls:", std::to_string(stats.drawCalls));
        Value("Triangles:", std::to_string(stats.triangles));
        Value("Debug lines:", std::to_string(stats.debugLines));
    }

    void WindowConfiguration::DrawCamera()
    {
        if (!ImGui::CollapsingHeader("Camera"))
            return;
        ModuleCamera3D* camera = App->camera;
        ImGui::SliderFloat("Vertical FOV", &camera->verticalFov, 20.0f, 120.0f, "%.0f deg");
        ImGui::DragFloat("Near plane", &camera->nearPlane, 0.01f, 0.01f, camera->farPlane - 0.01f);
        ImGui::DragFloat("Far plane", &camera->farPlane, 1.0f, camera->nearPlane + 0.01f, 100000.0f);
        ImGui::SliderFloat("Move speed", &camera->moveSpeed, 0.5f, 100.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Look sensitivity", &camera->lookSensitivity, 0.02f, 1.0f);
        ImGui::SliderFloat("Zoom speed", &camera->zoomSpeed, 0.02f, 0.5f);
        const glm::vec3& position = camera->GetPosition();
        const glm::vec3& reference = camera->GetReference();
        ImGui::Text("Position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
        ImGui::Text("Reference: %.2f, %.2f, %.2f", reference.x, reference.y, reference.z);
        if (ImGui::Button("Reset camera"))
            camera->Reset();
    }

    void WindowConfiguration::DrawInput()
    {
        if (!ImGui::CollapsingHeader("Input"))
            return;
        const ModuleInput* input = App->input;
        ImGui::Text("Mouse position: %.0f, %.0f", input->GetMouseX(), input->GetMouseY());
        ImGui::Text("Mouse motion: %.1f, %.1f", input->GetMouseMotionX(), input->GetMouseMotionY());
        ImGui::Text("Mouse wheel: %.1f", input->GetMouseWheel());
        ImGui::Text("Buttons: L %d  M %d  R %d", input->IsMouseButtonPressed(1), input->IsMouseButtonPressed(2), input->IsMouseButtonPressed(3));
        ImGui::Text("Alt %d  Shift %d", input->IsAltPressed(), input->IsShiftPressed());
    }

    void WindowConfiguration::DrawTextures()
    {
        if (!ImGui::CollapsingHeader("Textures & Import"))
            return;
        ModuleResources* resources = App->resources;
        TextureSettings settings = resources->GetTextureSettings();
        bool changed = ImGui::Checkbox("Linear filtering", &settings.linearFilter);
        changed |= ImGui::Checkbox("Mipmaps", &settings.mipmaps);
        changed |= ImGui::Checkbox("Repeat (wrap)", &settings.repeat);
        if (changed)
            resources->SetTextureSettings(settings);

        ImGui::SeparatorText("Model import (Assimp)");
        ModelImporter::Settings& import = ModelImporter::GetSettings();
        ImGui::Checkbox("Convert FBX units to meters", &import.convertUnitsToMeters);
        ImGui::Checkbox("Fix Blender -90 X axis", &import.fixBlenderAxis);
        ImGui::TextDisabled("(applied to the next imported models)");
        Value("Loaded textures:", std::to_string(resources->GetLoadedTextureCount()));
        Value("Texture memory:", Megabytes(resources->GetTextureMemoryBytes()));
    }

    void WindowConfiguration::DrawMemory()
    {
        if (!ImGui::CollapsingHeader("Memory"))
            return;
        char title[64];
        std::snprintf(title, sizeof(title), "Process RAM %.1f MB", memoryHistory.back());
        const float maxMemory = *std::max_element(memoryHistory.begin(), memoryHistory.end());
        ImGui::PlotLines("##memory", memoryHistory.data(), static_cast<int>(memoryHistory.size()), 0, title, 0.0f, maxMemory * 1.3f + 1.0f, ImVec2(0, 70));

        const ModuleResources* resources = App->resources;
        Value("Meshes in memory:", std::to_string(resources->GetLoadedMeshCount()));
        Value("Mesh memory:", Megabytes(resources->GetMeshMemoryBytes()));
        Value("GameObjects:", std::to_string(App->scene->CountGameObjects()));

        const SystemInfo::VideoMemory vram = SystemInfo::GetVideoMemory();
        if (vram.available)
        {
            if (vram.totalMB > 0)
                Value("VRAM total:", std::to_string(vram.totalMB) + " MB");
            Value("VRAM free:", std::to_string(vram.freeMB) + " MB");
        }
        else
            Value("VRAM:", "not reported by this driver");
    }

    void WindowConfiguration::DrawHardware()
    {
        if (!ImGui::CollapsingHeader("Hardware & Software"))
            return;
        const SystemInfo::Hardware& hardware = SystemInfo::GetHardware();
        const SystemInfo::Versions versions = SystemInfo::GetVersions(App->resources->GetDevILVersion());

        ImGui::SeparatorText("Software");
        Value("SDL:", versions.sdl);
        Value("OpenGL:", versions.opengl);
        Value("GLSL:", hardware.glslVersion);
        Value("DevIL:", versions.devil);
        Value("Assimp:", versions.assimp);
        Value("ImGui:", IMGUI_VERSION);
        Value("glm:", versions.glm);

        ImGui::SeparatorText("Hardware");
        Value("CPUs:", std::to_string(hardware.cpuCores) + " (Cache line: " + std::to_string(hardware.cpuCacheLineKB) + " bytes)");
        Value("System RAM:", std::to_string(hardware.systemRamMB / 1024.0f).substr(0, 4) + " GB");
        Value("Caps:", hardware.cpuFeatures);
        Value("GPU vendor:", hardware.gpuVendor);
        Value("GPU:", hardware.gpuRenderer);
    }
}
