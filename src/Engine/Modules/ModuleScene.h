#pragma once

#include "Engine/Core/Module.h"
#include "Engine/Importers/Primitives.h"

#include <memory>
#include <string>
#include <vector>

class GameObject;

// Holds the root of the GameObject tree, the current selection and handles
// dropped files (models are added to the scene, textures go to the selection).
class ModuleScene : public Module
{
public:
    ModuleScene();
    ~ModuleScene() override;

    bool Start() override;
    UpdateStatus Update(float dt) override;
    UpdateStatus PostUpdate(float dt) override;
    bool CleanUp() override;

    GameObject* GetRoot() const { return root.get(); }

    GameObject* CreateGameObject(const std::string& name, GameObject* parent = nullptr);
    GameObject* CreatePrimitive(PrimitiveType type, GameObject* parent = nullptr);
    GameObject* LoadModel(const std::string& path);
    GameObject* AddNextToScene(std::unique_ptr<GameObject> gameObject);
    bool ApplyTextureToSelection(const std::string& path);
    void HandleDroppedFile(const std::string& path);

    void DestroyGameObject(GameObject* gameObject); // deferred until the end of the frame
    bool Reparent(GameObject* gameObject, GameObject* newParent); // keeps the world transform
    void Clear();

    GameObject* GetSelected() const { return selected; }
    void SetSelected(GameObject* gameObject) { selected = gameObject; }

    std::size_t CountGameObjects() const;

private:
    std::unique_ptr<GameObject> root;
    GameObject* selected = nullptr;
    std::vector<GameObject*> pendingDestroy;
};
