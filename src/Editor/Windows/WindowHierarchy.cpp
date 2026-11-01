#include "Editor/Windows/WindowHierarchy.h"

#include "Engine/Core/Application.h"
#include "Engine/Modules/ModuleCamera3D.h"
#include "Engine/Modules/ModuleScene.h"
#include "Engine/Scene/ComponentMesh.h"
#include "Engine/Scene/GameObject.h"

#include <imgui.h>

#include <algorithm>
#include <cstdint>

namespace Editor
{
    namespace
    {
        constexpr const char* PAYLOAD = "NE_GAMEOBJECT";
    }

    void WindowHierarchy::Draw()
    {
        if (!ImGui::Begin(GetName().c_str(), OpenFlag()))
        {
            ImGui::End();
            return;
        }

        GameObject* root = App->scene->GetRoot();
        if (root)
        {
            ImGui::TextDisabled("%s (%zu objects)", root->GetName().c_str(), App->scene->CountGameObjects());
            ImGui::Separator();
            for (const auto& child : root->GetChildren())
                DrawNode(child.get());

            // Dropping on the empty area moves the object back to the root
            ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, std::max(ImGui::GetContentRegionAvail().y, 40.0f)));
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PAYLOAD))
                {
                    pendingChild = *static_cast<GameObject* const*>(payload->Data);
                    pendingParent = root;
                }
                ImGui::EndDragDropTarget();
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                App->scene->SetSelected(nullptr);
            if (ImGui::BeginPopupContextItem("HierarchyEmptyMenu"))
            {
                if (ImGui::MenuItem("Create Empty"))
                    App->scene->SetSelected(App->scene->CreateGameObject("GameObject"));
                ImGui::EndPopup();
            }
        }

        // Tree modifications are applied after iterating it
        if (pendingChild && pendingParent)
            App->scene->Reparent(pendingChild, pendingParent);
        if (pendingMove && pendingMove->GetParent())
            pendingMove->GetParent()->MoveChild(pendingMove, pendingOffset);
        pendingChild = pendingParent = pendingMove = nullptr;

        ImGui::End();
    }

    void WindowHierarchy::DrawNode(GameObject* gameObject)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (gameObject->GetChildren().empty())
            flags |= ImGuiTreeNodeFlags_Leaf;
        if (App->scene->GetSelected() == gameObject)
            flags |= ImGuiTreeNodeFlags_Selected;

        if (!gameObject->IsActiveInHierarchy())
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<std::uintptr_t>(gameObject->GetUID())), flags, "%s %s",
                                            gameObject->GetComponent<ComponentMesh>() ? "[M]" : "[ ]", gameObject->GetName().c_str());
        if (!gameObject->IsActiveInHierarchy())
            ImGui::PopStyleColor();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
            App->scene->SetSelected(gameObject);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            App->camera->Focus(gameObject->GetHierarchyAABB());

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(PAYLOAD, &gameObject, sizeof(GameObject*));
            ImGui::Text("Move %s", gameObject->GetName().c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PAYLOAD))
            {
                pendingChild = *static_cast<GameObject* const*>(payload->Data);
                pendingParent = gameObject;
            }
            ImGui::EndDragDropTarget();
        }

        DrawContextMenu(gameObject);

        if (open)
        {
            for (const auto& child : gameObject->GetChildren())
                DrawNode(child.get());
            ImGui::TreePop();
        }
    }

    void WindowHierarchy::DrawContextMenu(GameObject* gameObject)
    {
        if (!ImGui::BeginPopupContextItem())
            return;
        App->scene->SetSelected(gameObject);
        ImGui::TextDisabled("%s", gameObject->GetName().c_str());
        ImGui::Separator();
        if (ImGui::MenuItem("Create Empty Child"))
            App->scene->SetSelected(App->scene->CreateGameObject("GameObject", gameObject));
        if (ImGui::MenuItem("Move Up"))
        {
            pendingMove = gameObject;
            pendingOffset = -1;
        }
        if (ImGui::MenuItem("Move Down"))
        {
            pendingMove = gameObject;
            pendingOffset = 1;
        }
        if (ImGui::MenuItem("Focus", "F"))
            App->camera->Focus(gameObject->GetHierarchyAABB());
        ImGui::Separator();
        if (ImGui::MenuItem("Delete", "Supr"))
            App->scene->DestroyGameObject(gameObject);
        ImGui::EndPopup();
    }
}
