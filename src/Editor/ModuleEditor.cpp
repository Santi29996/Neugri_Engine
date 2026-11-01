#include "Editor/ModuleEditor.h"

#include "Editor/EditorWindow.h"
#include "Editor/Windows/WindowAbout.h"
#include "Editor/Windows/WindowConfiguration.h"
#include "Editor/Windows/WindowConsole.h"
#include "Editor/Windows/WindowHierarchy.h"
#include "Editor/Windows/WindowInspector.h"
#include "Editor/Windows/WindowScene.h"

#include "Engine/Core/Application.h"
#include "Engine/Core/EngineInfo.h"
#include "Engine/Core/Log.h"
#include "Engine/Importers/Primitives.h"
#include "Engine/Modules/ModuleCamera3D.h"
#include "Engine/Modules/ModuleInput.h"
#include "Engine/Modules/ModuleRenderer3D.h"
#include "Engine/Modules/ModuleResources.h"
#include "Engine/Modules/ModuleScene.h"
#include "Engine/Modules/ModuleWindow.h"
#include "Engine/Scene/GameObject.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <imgui_internal.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace Editor
{
    ModuleEditor::ModuleEditor() : Module("Editor") {}

    ModuleEditor::~ModuleEditor() = default;

    bool ModuleEditor::Init()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
        io.IniFilename = "imgui.ini";

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.TabRounding = 3.0f;
        style.Colors[ImGuiCol_Header] = ImVec4(0.85f, 0.55f, 0.10f, 0.45f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.85f, 0.55f, 0.10f, 0.65f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.85f, 0.55f, 0.10f, 0.85f);

        ImGui_ImplSDL3_InitForOpenGL(App->window->GetWindow(), App->window->GetContext());
        ImGui_ImplOpenGL3_Init("#version 330");

        App->input->AddEventListener([](const SDL_Event& event) { ImGui_ImplSDL3_ProcessEvent(&event); });
        App->renderer->SetRenderToFramebuffer(true);

        AddWindow<WindowScene>();
        AddWindow<WindowHierarchy>();
        AddWindow<WindowInspector>();
        AddWindow<WindowConsole>();
        AddWindow<WindowConfiguration>();
        AddWindow<WindowAbout>()->SetOpen(false);

        LOG("ImGui %s initialized (docking, SDL3 + OpenGL3 backends)", IMGUI_VERSION);
        return true;
    }

    UpdateStatus ModuleEditor::PostUpdate(float /*dt*/)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        DrawDockspace();
        DrawMainMenuBar();
        HandleShortcuts();

        for (const auto& window : windows)
            if (window->IsOpen())
                window->Draw();

        // Without the Scene window the camera must not react
        if (const EditorWindow* scene = FindWindow("Scene"); scene && !scene->IsOpen())
        {
            App->camera->SetMouseInputEnabled(false);
            App->camera->SetKeyboardInputEnabled(false);
        }

        if (showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);

        ImGui::Render();
        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels(App->window->GetWindow(), &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.08f, 0.08f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        return quitRequested ? UpdateStatus::Stop : UpdateStatus::Continue;
    }

    void ModuleEditor::DrawDockspace()
    {
        const ImGuiID dockspace = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

        // Default layout the first time the editor runs (no imgui.ini yet)
        if (!layoutInitialized)
        {
            layoutInitialized = true;
            const bool forced = layoutResetRequested;
            layoutResetRequested = false;
            const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace);
            if (forced || node == nullptr || node->IsLeafNode())
            {
                ImGui::DockBuilderRemoveNode(dockspace);
                ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace, ImGui::GetMainViewport()->WorkSize);

                ImGuiID center = dockspace;
                const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.18f, nullptr, &center);
                const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.26f, nullptr, &center);
                const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.28f, nullptr, &center);

                ImGui::DockBuilderDockWindow("Hierarchy", left);
                ImGui::DockBuilderDockWindow("Inspector", right);
                ImGui::DockBuilderDockWindow("Console", bottom);
                ImGui::DockBuilderDockWindow("Configuration", bottom);
                ImGui::DockBuilderDockWindow("Scene", center);
                ImGui::DockBuilderFinish(dockspace);
            }
        }
    }

    void ModuleEditor::DrawMainMenuBar()
    {
        if (!ImGui::BeginMainMenuBar())
            return;

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene"))
                App->scene->Clear();
            if (ImGui::BeginMenu("Load from Assets"))
            {
                DrawAssetsMenu();
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4"))
                quitRequested = true;
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            for (const auto& window : windows)
                ImGui::MenuItem(window->GetName().c_str(), nullptr, window->OpenFlag());
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow);
            if (ImGui::MenuItem("Reset layout"))
            {
                layoutInitialized = false;
                layoutResetRequested = true;
                for (const auto& window : windows)
                    window->SetOpen(window->GetName() != "About");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject"))
        {
            GameObject* parent = nullptr; // primitives go to the root
            if (ImGui::MenuItem("Create Empty"))
                App->scene->SetSelected(App->scene->CreateGameObject("GameObject", parent));
            ImGui::Separator();
            constexpr PrimitiveType primitives[] = { PrimitiveType::Cube, PrimitiveType::Sphere, PrimitiveType::Cylinder,
                                                     PrimitiveType::Pyramid, PrimitiveType::Plane, PrimitiveType::Torus };
            for (const PrimitiveType type : primitives)
                if (ImGui::MenuItem(Primitives::GetName(type)))
                    App->scene->CreatePrimitive(type, parent);
            ImGui::Separator();
            const bool hasSelection = App->scene->GetSelected() != nullptr;
            if (ImGui::MenuItem("Focus selected", "F", false, hasSelection))
                App->camera->Focus(App->scene->GetSelected()->GetHierarchyAABB());
            if (ImGui::MenuItem("Delete selected", "Supr", false, hasSelection))
                App->scene->DestroyGameObject(App->scene->GetSelected());
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            const std::string repository(EngineInfo::REPOSITORY_URL);
            if (ImGui::MenuItem("GitHub Documentation"))
                SDL_OpenURL((repository + "/docs").c_str());
            if (ImGui::MenuItem("Report a bug"))
                SDL_OpenURL((repository + "/issues").c_str());
            if (ImGui::MenuItem("Download latest"))
                SDL_OpenURL((repository + "/releases").c_str());
            ImGui::Separator();
            if (EditorWindow* about = FindWindow("About"))
                ImGui::MenuItem("About", nullptr, about->OpenFlag());
            ImGui::EndMenu();
        }

        // Right aligned FPS counter
        const std::string fps = std::to_string(static_cast<int>(App->GetFPS())) + " FPS";
        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(fps.c_str()).x - 16.0f);
        ImGui::TextDisabled("%s", fps.c_str());

        ImGui::EndMainMenuBar();
    }

    void ModuleEditor::DrawAssetsMenu()
    {
        // Same path as a drag & drop: models are added, textures go to the selection
        std::error_code error;
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator("Assets", error))
            if (entry.is_regular_file())
                files.push_back(entry.path());
        std::sort(files.begin(), files.end());

        ImGui::TextDisabled("Models");
        for (const auto& file : files)
            if (ModuleResources::IsModelFile(file.string()) && ImGui::MenuItem(file.generic_string().c_str()))
                App->scene->HandleDroppedFile(file.string());
        ImGui::Separator();
        ImGui::TextDisabled("Textures (applied to the selection)");
        const bool hasSelection = App->scene->GetSelected() != nullptr;
        for (const auto& file : files)
            if (ModuleResources::IsTextureFile(file.string()) && ImGui::MenuItem(file.generic_string().c_str(), nullptr, false, hasSelection))
                App->scene->HandleDroppedFile(file.string());
    }

    void ModuleEditor::HandleShortcuts()
    {
        const ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput)
            return;
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
            if (GameObject* selected = App->scene->GetSelected())
                App->scene->DestroyGameObject(selected);
        if (ImGui::IsKeyPressed(ImGuiKey_F12, false))
            App->RequestScreenshot("screenshot.bmp");
    }

    EditorWindow* ModuleEditor::FindWindow(const char* name) const
    {
        for (const auto& window : windows)
            if (window->GetName() == name)
                return window.get();
        return nullptr;
    }

    bool ModuleEditor::CleanUp()
    {
        windows.clear();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        LOG("ImGui shut down");
        return true;
    }
}
