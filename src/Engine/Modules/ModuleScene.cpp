#include "Engine/Modules/ModuleScene.h"

#include "Engine/Core/Application.h"
#include "Engine/Core/Log.h"
#include "Engine/Importers/ModelImporter.h"
#include "Engine/Modules/ModuleCamera3D.h"
#include "Engine/Modules/ModuleInput.h"
#include "Engine/Modules/ModuleResources.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Scene/ComponentMaterial.h"
#include "Engine/Scene/ComponentMesh.h"
#include "Engine/Scene/ComponentTransform.h"
#include "Engine/Scene/GameObject.h"

#include <algorithm>
#include <filesystem>
#include <functional>

ModuleScene::ModuleScene() : Module("Scene") {}

ModuleScene::~ModuleScene() = default;

bool ModuleScene::Start()
{
    root = std::make_unique<GameObject>("Scene");

    // The Baker House is always loaded at startup
    const std::string startModel = "Assets/BakerHouse.fbx";
    if (GameObject* house = LoadModel(startModel))
    {
        selected = house;
        App->camera->Focus(house->GetHierarchyAABB());
    }
    else
        LOG_ERROR("Start model %s could not be loaded (is the Assets folder next to the executable?)", startModel.c_str());
    return true;
}

UpdateStatus ModuleScene::Update(float dt)
{
    for (const std::string& file : App->input->TakeDroppedFiles())
        HandleDroppedFile(file);

    root->Update(dt);
    return UpdateStatus::Continue;
}

UpdateStatus ModuleScene::PostUpdate(float /*dt*/)
{
    for (GameObject* gameObject : pendingDestroy)
    {
        if (selected == gameObject || (selected && selected->IsDescendantOf(gameObject)))
            selected = nullptr;
        if (GameObject* parent = gameObject->GetParent())
            parent->DetachChild(gameObject); // the returned unique_ptr destroys it
    }
    pendingDestroy.clear();
    return UpdateStatus::Continue;
}

bool ModuleScene::CleanUp()
{
    selected = nullptr;
    pendingDestroy.clear();
    root.reset();
    return true;
}

void ModuleScene::HandleDroppedFile(const std::string& path)
{
    if (ModuleResources::IsModelFile(path))
    {
        if (GameObject* model = LoadModel(path))
        {
            selected = model;
            App->camera->Focus(model->GetHierarchyAABB());
        }
    }
    else if (ModuleResources::IsTextureFile(path))
        ApplyTextureToSelection(path);
    else
        LOG_WARNING("Unsupported file type: %s", path.c_str());
}

GameObject* ModuleScene::CreateGameObject(const std::string& name, GameObject* parent)
{
    GameObject* target = parent ? parent : root.get();
    return target->AddChild(std::make_unique<GameObject>(name));
}

GameObject* ModuleScene::CreatePrimitive(PrimitiveType type, GameObject* parent)
{
    std::shared_ptr<Mesh> mesh = Primitives::Create(type);
    App->resources->RegisterMesh(mesh);

    auto gameObject = std::make_unique<GameObject>(Primitives::GetName(type));
    gameObject->AddComponent<ComponentMesh>()->SetMesh(std::move(mesh));
    gameObject->AddComponent<ComponentMaterial>();
    GameObject* created = parent ? parent->AddChild(std::move(gameObject)) : AddNextToScene(std::move(gameObject));
    selected = created;
    LOG("Primitive %s created", Primitives::GetName(type));
    return created;
}

GameObject* ModuleScene::LoadModel(const std::string& path)
{
    std::unique_ptr<GameObject> model = ModelImporter::Import(path);
    if (!model)
        return nullptr;
    return AddNextToScene(std::move(model));
}

GameObject* ModuleScene::AddNextToScene(std::unique_ptr<GameObject> gameObject)
{
    // Place the new object next to what is already in the scene, standing on the ground
    const AABB sceneBox = root->GetHierarchyAABB();
    GameObject* added = root->AddChild(std::move(gameObject));
    const AABB box = added->GetHierarchyAABB();
    if (sceneBox.IsValid() && box.IsValid())
    {
        const float gap = 0.25f * std::max(sceneBox.Size().y, box.Size().y);
        ComponentTransform* transform = added->GetTransform();
        glm::vec3 position = transform->GetPosition();
        position.x += sceneBox.max.x + gap - box.min.x;
        position.y -= box.min.y - 0.001f; // tiny lift avoids z-fighting with the grid
        position.z += sceneBox.Center().z - box.Center().z;
        transform->SetPosition(position);
    }
    return added;
}

bool ModuleScene::ApplyTextureToSelection(const std::string& path)
{
    if (!selected)
    {
        LOG_WARNING("Select a GameObject in the hierarchy before dropping a texture");
        return false;
    }
    std::shared_ptr<Texture> texture = App->resources->LoadTexture(path);
    if (!texture)
        return false;

    // The texture goes to the selected object and all its children with a mesh
    int applied = 0;
    std::function<void(GameObject*)> apply = [&](GameObject* gameObject) {
        if (gameObject->GetComponent<ComponentMesh>())
        {
            gameObject->GetOrAddComponent<ComponentMaterial>()->SetTexture(texture);
            ++applied;
        }
        for (const auto& child : gameObject->GetChildren())
            apply(child.get());
    };
    apply(selected);
    LOG("Texture %s applied to %d GameObject(s) of '%s'", path.c_str(), applied, selected->GetName().c_str());
    return applied > 0;
}

void ModuleScene::DestroyGameObject(GameObject* gameObject)
{
    if (gameObject && gameObject != root.get())
        pendingDestroy.push_back(gameObject);
}

bool ModuleScene::Reparent(GameObject* gameObject, GameObject* newParent)
{
    if (!gameObject || gameObject == root.get() || !newParent || newParent == gameObject || newParent->IsDescendantOf(gameObject))
        return false;
    GameObject* oldParent = gameObject->GetParent();
    if (oldParent == newParent)
        return false;

    const glm::mat4 world = gameObject->GetTransform()->GetWorldMatrix();
    std::unique_ptr<GameObject> detached = oldParent->DetachChild(gameObject);
    newParent->AddChild(std::move(detached));
    gameObject->GetTransform()->SetWorldMatrix(world);
    return true;
}

void ModuleScene::Clear()
{
    selected = nullptr;
    pendingDestroy.clear();
    root = std::make_unique<GameObject>("Scene");
    LOG("Scene cleared");
}

std::size_t ModuleScene::CountGameObjects() const
{
    std::function<std::size_t(const GameObject*)> count = [&](const GameObject* node) {
        std::size_t total = 1;
        for (const auto& child : node->GetChildren())
            total += count(child.get());
        return total;
    };
    return root ? count(root.get()) - 1 : 0;
}
