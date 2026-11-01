#pragma once

#include "Engine/Resources/AABB.h"
#include "Engine/Scene/Component.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class ComponentTransform;

// GameObject = name + tree node + 0..N components (Transform always exists)
class GameObject
{
public:
    explicit GameObject(std::string name);
    ~GameObject();

    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

    void Update(float dt);

    // ---- Components
    template<typename T>
    T* AddComponent()
    {
        auto component = std::make_unique<T>(this);
        T* raw = component.get();
        components.push_back(std::move(component));
        return raw;
    }

    template<typename T>
    T* GetComponent() const
    {
        for (const auto& component : components)
            if (component->GetType() == T::TYPE)
                return static_cast<T*>(component.get());
        return nullptr;
    }

    template<typename T>
    T* GetOrAddComponent()
    {
        T* component = GetComponent<T>();
        return component ? component : AddComponent<T>();
    }

    const std::vector<std::unique_ptr<Component>>& GetComponents() const { return components; }
    ComponentTransform* GetTransform() const { return transform; }

    // ---- Hierarchy
    GameObject* GetParent() const { return parent; }
    const std::vector<std::unique_ptr<GameObject>>& GetChildren() const { return children; }
    GameObject* AddChild(std::unique_ptr<GameObject> child);
    std::unique_ptr<GameObject> DetachChild(GameObject* child);
    bool IsDescendantOf(const GameObject* other) const;
    bool MoveChild(GameObject* child, int offset); // reorder inside the children list

    // ---- Properties
    const std::string& GetName() const { return name; }
    void SetName(std::string newName) { name = std::move(newName); }
    bool IsActive() const { return active; }
    bool IsActiveInHierarchy() const;
    void SetActive(bool enabled) { active = enabled; }
    std::uint32_t GetUID() const { return uid; }

    // World AABB of this object and all its children
    AABB GetHierarchyAABB() const;

private:
    std::string name;
    std::uint32_t uid = 0;
    bool active = true;

    GameObject* parent = nullptr;
    std::vector<std::unique_ptr<GameObject>> children;
    std::vector<std::unique_ptr<Component>> components;
    ComponentTransform* transform = nullptr;
};
