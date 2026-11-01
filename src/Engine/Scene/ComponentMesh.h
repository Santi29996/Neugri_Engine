#pragma once

#include "Engine/Resources/AABB.h"
#include "Engine/Scene/Component.h"

#include <memory>

class Mesh;

class ComponentMesh : public Component
{
public:
    static constexpr ComponentType TYPE = ComponentType::Mesh;

    explicit ComponentMesh(GameObject* owner) : Component(owner, TYPE) {}

    const std::shared_ptr<Mesh>& GetMesh() const { return mesh; }
    void SetMesh(std::shared_ptr<Mesh> newMesh) { mesh = std::move(newMesh); }

    AABB GetWorldAABB() const;

    bool showVertexNormals = false;
    bool showFaceNormals = false;

private:
    std::shared_ptr<Mesh> mesh;
};
