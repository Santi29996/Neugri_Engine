#include "Editor/Windows/WindowScene.h"

#include "Engine/Core/Application.h"
#include "Engine/Modules/ModuleCamera3D.h"
#include "Engine/Modules/ModuleRenderer3D.h"
#include "Engine/Modules/ModuleScene.h"
#include "Engine/Resources/Framebuffer.h"
#include "Engine/Scene/GameObject.h"

#include <imgui.h>

#include <cstdint>

namespace Editor
{
    void WindowScene::Draw()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        const bool visible = ImGui::Begin(GetName().c_str(), OpenFlag(), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();

        ModuleCamera3D* camera = App->camera;
        if (!visible)
        {
            camera->SetMouseInputEnabled(false);
            camera->SetKeyboardInputEnabled(false);
            ImGui::End();
            return;
        }

        const ImVec2 size = ImGui::GetContentRegionAvail();
        const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
        App->renderer->SetViewportSize(static_cast<int>(size.x * scale.x), static_cast<int>(size.y * scale.y));

        const ImVec2 origin = ImGui::GetCursorScreenPos();
        if (const Framebuffer* framebuffer = App->renderer->GetFramebuffer(); framebuffer && framebuffer->GetColorTexture() != 0)
        {
            const auto textureId = static_cast<ImTextureID>(static_cast<std::uintptr_t>(framebuffer->GetColorTexture()));
            ImGui::Image(textureId, size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f)); // OpenGL textures are upside down
        }
        const bool hovered = ImGui::IsItemHovered();

        // Clicking in the scene gives it focus (so keyboard shortcuts go to the camera)
        if (hovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
            ImGui::SetWindowFocus();

        const bool typing = ImGui::GetIO().WantTextInput;
        camera->SetMouseInputEnabled(hovered);
        camera->SetKeyboardInputEnabled(!typing && (hovered || ImGui::IsWindowFocused() || camera->IsInteracting()));

        // Overlay with a short controls reminder
        ImGui::SetCursorScreenPos(ImVec2(origin.x + 8.0f, origin.y + 6.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.75f));
        if (showHelp)
        {
            ImGui::TextUnformatted("RMB + WASD/QE: fly   Wheel: zoom   Alt+LMB: orbit   MMB: pan   F: focus   Shift: x2");
            const GameObject* selected = App->scene->GetSelected();
            ImGui::SetCursorScreenPos(ImVec2(origin.x + 8.0f, ImGui::GetCursorScreenPos().y));
            ImGui::Text("Selected: %s   |   Drop .fbx/.obj to import, .png/.dds to texture the selection", selected ? selected->GetName().c_str() : "none");
        }
        ImGui::PopStyleColor();
        ImGui::SetCursorScreenPos(ImVec2(origin.x + size.x - 70.0f, origin.y + 4.0f));
        ImGui::Checkbox("Help", &showHelp);

        ImGui::End();
    }
}
