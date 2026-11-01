#pragma once

#include "Editor/EditorWindow.h"

class GameObject;

namespace Editor
{
    // GameObject tree: select, drag & drop to change the parent, right click menu
    class WindowHierarchy : public EditorWindow
    {
    public:
        WindowHierarchy() : EditorWindow("Hierarchy") {}
        void Draw() override;

    private:
        void DrawNode(GameObject* gameObject);
        void DrawContextMenu(GameObject* gameObject);

        GameObject* pendingParent = nullptr; // reparent requests are applied after drawing the tree
        GameObject* pendingChild = nullptr;
        GameObject* pendingMove = nullptr;
        int pendingOffset = 0;
    };
}
