#pragma once

#include "Engine/Scene/Component.h"

#include <glm/glm.hpp>

#include <memory>

class Texture;

// Diffuse channel of the object (texture + tint color)
class ComponentMaterial : public Component
{
public:
    static constexpr ComponentType TYPE = ComponentType::Material;

    explicit ComponentMaterial(GameObject* owner) : Component(owner, TYPE) {}

    const std::shared_ptr<Texture>& GetTexture() const { return texture; }
    void SetTexture(std::shared_ptr<Texture> newTexture) { texture = std::move(newTexture); }

    glm::vec4 color { 1.0f };
    bool showCheckers = false; // draw the black & white checkers texture instead of the diffuse one

private:
    std::shared_ptr<Texture> texture;
};
