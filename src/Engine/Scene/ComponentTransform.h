#pragma once

#include "Engine/Scene/Component.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Position / rotation / scale kept separated and combined into a matrix when needed.
// The world matrix uses lazy evaluation with a dirty flag: global = parent global * local.
class ComponentTransform : public Component
{
public:
    static constexpr ComponentType TYPE = ComponentType::Transform;

    explicit ComponentTransform(GameObject* owner);

    const glm::vec3& GetPosition() const { return position; }
    const glm::quat& GetRotation() const { return rotation; }
    const glm::vec3& GetScale() const { return scale; }
    const glm::vec3& GetEulerDegrees() const { return eulerDegrees; }

    void SetPosition(const glm::vec3& value);
    void SetRotation(const glm::quat& value);
    void SetEulerDegrees(const glm::vec3& degrees);
    void SetScale(const glm::vec3& value);
    void SetLocal(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
    void Reset();

    const glm::mat4& GetLocalMatrix() const;
    const glm::mat4& GetWorldMatrix() const;

    // Keeps the object in the same world place when it changes parent
    void SetWorldMatrix(const glm::mat4& world);

    glm::vec3 GetWorldPosition() const { return glm::vec3(GetWorldMatrix()[3]); }

    void MarkDirty();      // local values changed
    void MarkWorldDirty(); // parent changed or moved

private:
    glm::vec3 position { 0.0f };
    glm::quat rotation { 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale { 1.0f };
    glm::vec3 eulerDegrees { 0.0f }; // cached for the inspector to avoid euler <-> quat jitter

    mutable glm::mat4 localMatrix { 1.0f };
    mutable glm::mat4 worldMatrix { 1.0f };
    mutable bool localDirty = true;
    mutable bool worldDirty = true;
};
