#pragma once

#include "Editor/EditorWindow.h"

class GameObject;
class ComponentTransform;
class ComponentMesh;
class ComponentMaterial;

namespace Editor
{
    // Components of the selected GameObject. The editor draws each component type
    // so the engine components do not depend on ImGui.
    class WindowInspector : public EditorWindow
    {
    public:
        WindowInspector() : EditorWindow("Inspector") {}
        void Draw() override;

    private:
        void DrawHeader(GameObject& gameObject);
        void DrawTransform(ComponentTransform& transform);
        void DrawMesh(ComponentMesh& mesh);
        void DrawMaterial(ComponentMaterial& material);
        void DrawAddComponent(GameObject& gameObject);

        const GameObject* lastSelected = nullptr;
        char nameBuffer[128] = {};
    };
}
