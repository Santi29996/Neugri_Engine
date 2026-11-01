#include "Engine/Scene/GameObject.h"

#include "Engine/Scene/ComponentMesh.h"
#include "Engine/Scene/ComponentTransform.h"

#include <algorithm>
#include <random>

namespace
{
    std::uint32_t GenerateUID()
    {
        static std::mt19937 generator { std::random_device {}() };
        static std::uniform_int_distribution<std::uint32_t> distribution(1u, 0xFFFFFFFFu);
        return distribution(generator);
    }
}

GameObject::GameObject(std::string goName) : name(std::move(goName)), uid(GenerateUID())
{
    transform = AddComponent<ComponentTransform>();
}

GameObject::~GameObject() = default;

void GameObject::Update(float dt)
{
    if (!active)
        return;
    for (const auto& component : components)
        if (component->IsActive())
            component->Update(dt);
    for (const auto& child : children)
        child->Update(dt);
}

GameObject* GameObject::AddChild(std::unique_ptr<GameObject> child)
{
    GameObject* raw = child.get();
    raw->parent = this;
    children.push_back(std::move(child));
    raw->GetTransform()->MarkWorldDirty();
    return raw;
}

std::unique_ptr<GameObject> GameObject::DetachChild(GameObject* child)
{
    const auto it = std::find_if(children.begin(), children.end(),
        [child](const std::unique_ptr<GameObject>& candidate) { return candidate.get() == child; });
    if (it == children.end())
        return nullptr;

    std::unique_ptr<GameObject> detached = std::move(*it);
    children.erase(it);
    detached->parent = nullptr;
    detached->GetTransform()->MarkWorldDirty();
    return detached;
}

bool GameObject::IsDescendantOf(const GameObject* other) const
{
    for (const GameObject* node = parent; node != nullptr; node = node->parent)
        if (node == other)
            return true;
    return false;
}

bool GameObject::MoveChild(GameObject* child, int offset)
{
    const auto it = std::find_if(children.begin(), children.end(),
        [child](const std::unique_ptr<GameObject>& candidate) { return candidate.get() == child; });
    if (it == children.end())
        return false;
    const auto index = std::distance(children.begin(), it);
    const auto target = index + offset;
    if (target < 0 || target >= static_cast<std::ptrdiff_t>(children.size()))
        return false;
    std::swap(children[index], children[target]);
    return true;
}

bool GameObject::IsActiveInHierarchy() const
{
    for (const GameObject* node = this; node != nullptr; node = node->parent)
        if (!node->active)
            return false;
    return true;
}

AABB GameObject::GetHierarchyAABB() const
{
    AABB result;
    if (const ComponentMesh* mesh = GetComponent<ComponentMesh>())
        result.Enclose(mesh->GetWorldAABB());
    for (const auto& child : children)
        result.Enclose(child->GetHierarchyAABB());
    return result;
}
