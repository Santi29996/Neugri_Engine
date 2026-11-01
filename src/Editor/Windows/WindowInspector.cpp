#include "Editor/Windows/WindowInspector.h"

#include "Engine/Core/Application.h"
#include "Engine/Modules/ModuleRenderer3D.h"
#include "Engine/Modules/ModuleResources.h"
#include "Engine/Modules/ModuleScene.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Texture.h"
#include "Engine/Scene/ComponentMaterial.h"
#include "Engine/Scene/ComponentMesh.h"
#include "Engine/Scene/ComponentTransform.h"
#include "Engine/Scene/GameObject.h"

#include <imgui.h>

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstring>

namespace Editor
{
    namespace
    {
        // Collapsing header with an "active" checkbox inside
        bool ComponentHeader(const char* label, Component& component)
        {
            const bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
            ImGui::SameLine(ImGui::GetContentRegionMax().x - 24.0f);
            bool active = component.IsActive();
            ImGui::PushID(label);
            if (ImGui::Checkbox("##active", &active))
                component.SetActive(active);
            ImGui::PopID();
            return open;
        }

        void Field(const char* label, const char* format, ...)
        {
            ImGui::TextDisabled("%s", label);
            ImGui::SameLine(110.0f);
            va_list args;
            va_start(args, format);
            ImGui::TextV(format, args);
            va_end(args);
        }
    }

    void WindowInspector::Draw()
    {
        if (!ImGui::Begin(GetName().c_str(), OpenFlag()))
        {
            ImGui::End();
            return;
        }

        GameObject* selected = App->scene->GetSelected();
        if (!selected)
        {
            ImGui::TextDisabled("Select a GameObject in the Hierarchy");
            lastSelected = nullptr;
            ImGui::End();
            return;
        }

        DrawHeader(*selected);
        if (ComponentTransform* transform = selected->GetTransform())
            DrawTransform(*transform);
        if (ComponentMesh* mesh = selected->GetComponent<ComponentMesh>())
            DrawMesh(*mesh);
        if (ComponentMaterial* material = selected->GetComponent<ComponentMaterial>())
            DrawMaterial(*material);
        DrawAddComponent(*selected);

        ImGui::End();
    }

    void WindowInspector::DrawHeader(GameObject& gameObject)
    {
        if (lastSelected != &gameObject)
        {
            lastSelected = &gameObject;
            std::strncpy(nameBuffer, gameObject.GetName().c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        }

        bool active = gameObject.IsActive();
        if (ImGui::Checkbox("##goactive", &active))
            gameObject.SetActive(active);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit())
            if (nameBuffer[0] != '\0')
                gameObject.SetName(nameBuffer);
        ImGui::TextDisabled("UID: %u", gameObject.GetUID());
        ImGui::Spacing();
    }

    void WindowInspector::DrawTransform(ComponentTransform& transform)
    {
        if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        glm::vec3 position = transform.GetPosition();
        if (ImGui::DragFloat3("Position", &position.x, 0.05f))
            transform.SetPosition(position);
        glm::vec3 rotation = transform.GetEulerDegrees() + glm::vec3(0.0f); // + 0 turns -0.000 into 0.000
        if (ImGui::DragFloat3("Rotation", &rotation.x, 0.5f))
            transform.SetEulerDegrees(rotation);
        glm::vec3 scale = transform.GetScale();
        if (ImGui::DragFloat3("Scale", &scale.x, 0.01f))
            transform.SetScale(scale);
        if (ImGui::Button("Reset transform"))
            transform.Reset();

        const glm::vec3 world = transform.GetWorldPosition();
        ImGui::TextDisabled("World position: %.2f, %.2f, %.2f", world.x, world.y, world.z);
    }

    void WindowInspector::DrawMesh(ComponentMesh& component)
    {
        if (!ComponentHeader("Mesh", component))
            return;
        const std::shared_ptr<Mesh>& mesh = component.GetMesh();
        if (!mesh)
        {
            ImGui::TextDisabled("No mesh");
            return;
        }
        Field("Name:", "%s", mesh->GetName().c_str());
        Field("Source:", "%s", mesh->GetSourcePath().empty() ? "<procedural>" : mesh->GetSourcePath().c_str());
        Field("Vertices:", "%zu", mesh->GetVertices().size());
        Field("Indices:", "%zu", mesh->GetIndices().size());
        Field("Triangles:", "%zu", mesh->GetTriangleCount());
        Field("Normals:", "%s", mesh->HasNormals() ? "yes" : "no");
        Field("UVs:", "%s", mesh->HasUVs() ? "yes" : "no");
        Field("Memory:", "%.1f KB", static_cast<double>(mesh->GetMemoryBytes()) / 1024.0);
        const glm::vec3 size = mesh->GetAABB().Size();
        Field("AABB size:", "%.2f x %.2f x %.2f", size.x, size.y, size.z);

        ImGui::Spacing();
        ImGui::Checkbox("Vertex normals", &component.showVertexNormals);
        ImGui::SameLine();
        ImGui::Checkbox("Face normals", &component.showFaceNormals);
        if (component.showVertexNormals || component.showFaceNormals)
            ImGui::SliderFloat("Length", &App->renderer->settings.normalsLength, 0.01f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
    }

    void WindowInspector::DrawMaterial(ComponentMaterial& material)
    {
        if (!ComponentHeader("Texture", material))
            return;
        const std::shared_ptr<Texture>& texture = material.GetTexture();
        if (texture)
        {
            Field("Path:", "%s", texture->GetPath().c_str());
            Field("Size:", "%d x %d px", texture->GetWidth(), texture->GetHeight());
            Field("Memory:", "%.1f KB", static_cast<double>(texture->GetMemoryBytes()) / 1024.0);
            const float width = std::min(ImGui::GetContentRegionAvail().x, 192.0f);
            const float height = width * texture->GetHeight() / std::max(texture->GetWidth(), 1);
            ImGui::Image(static_cast<ImTextureID>(static_cast<std::uintptr_t>(texture->GetId())), ImVec2(width, height), ImVec2(0, 1), ImVec2(1, 0));
            if (ImGui::Button("Remove texture"))
                material.SetTexture(nullptr);
        }
        else
            ImGui::TextDisabled("No diffuse texture (drop a .png/.dds on the window)");

        ImGui::Checkbox("Show checkers texture", &material.showCheckers);
        ImGui::ColorEdit4("Tint", &material.color.x);
    }

    void WindowInspector::DrawAddComponent(GameObject& gameObject)
    {
        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Add Component", ImVec2(-1.0f, 0.0f)))
            ImGui::OpenPopup("AddComponentPopup");
        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            if (ImGui::MenuItem("Mesh", nullptr, false, gameObject.GetComponent<ComponentMesh>() == nullptr))
                gameObject.AddComponent<ComponentMesh>();
            if (ImGui::MenuItem("Texture (Material)", nullptr, false, gameObject.GetComponent<ComponentMaterial>() == nullptr))
                gameObject.AddComponent<ComponentMaterial>();
            ImGui::EndPopup();
        }
    }
}
