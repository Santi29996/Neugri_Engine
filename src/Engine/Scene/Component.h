#pragma once

class GameObject;

enum class ComponentType { Transform, Mesh, Material };

class Component
{
public:
    Component(GameObject* owner, ComponentType type) : owner(owner), type(type) {}
    virtual ~Component() = default;

    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    virtual void Update(float /*dt*/) {}

    ComponentType GetType() const { return type; }
    GameObject* GetOwner() const { return owner; }

    bool IsActive() const { return active; }
    void SetActive(bool enabled) { active = enabled; }

private:
    GameObject* owner = nullptr;
    ComponentType type;
    bool active = true;
};
