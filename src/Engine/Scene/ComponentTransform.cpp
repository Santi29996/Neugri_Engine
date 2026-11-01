#include "Engine/Scene/ComponentTransform.h"

#include "Engine/Scene/GameObject.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

ComponentTransform::ComponentTransform(GameObject* owner) : Component(owner, TYPE) {}

void ComponentTransform::SetPosition(const glm::vec3& value)
{
    position = value;
    MarkDirty();
}

void ComponentTransform::SetRotation(const glm::quat& value)
{
    rotation = glm::normalize(value);
    eulerDegrees = glm::degrees(glm::eulerAngles(rotation));
    MarkDirty();
}

void ComponentTransform::SetEulerDegrees(const glm::vec3& degrees)
{
    eulerDegrees = degrees;
    rotation = glm::quat(glm::radians(degrees));
    MarkDirty();
}

void ComponentTransform::SetScale(const glm::vec3& value)
{
    scale = value;
    MarkDirty();
}

void ComponentTransform::SetLocal(const glm::vec3& newPosition, const glm::quat& newRotation, const glm::vec3& newScale)
{
    position = newPosition;
    scale = newScale;
    SetRotation(newRotation); // also marks dirty
}

void ComponentTransform::Reset()
{
    SetLocal(glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f));
}

const glm::mat4& ComponentTransform::GetLocalMatrix() const
{
    if (localDirty)
    {
        localMatrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
        localDirty = false;
    }
    return localMatrix;
}

const glm::mat4& ComponentTransform::GetWorldMatrix() const
{
    if (worldDirty)
    {
        const GameObject* parent = GetOwner()->GetParent();
        worldMatrix = parent ? parent->GetTransform()->GetWorldMatrix() * GetLocalMatrix() : GetLocalMatrix();
        worldDirty = false;
    }
    return worldMatrix;
}

void ComponentTransform::SetWorldMatrix(const glm::mat4& world)
{
    const GameObject* parent = GetOwner()->GetParent();
    const glm::mat4 local = parent ? glm::inverse(parent->GetTransform()->GetWorldMatrix()) * world : world;

    glm::vec3 newScale, newPosition, skew;
    glm::vec4 perspective;
    glm::quat newRotation;
    if (glm::decompose(local, newScale, newRotation, newPosition, skew, perspective))
        SetLocal(newPosition, newRotation, newScale);
}

void ComponentTransform::MarkDirty()
{
    localDirty = true;
    MarkWorldDirty();
}

void ComponentTransform::MarkWorldDirty()
{
    worldDirty = true;
    for (const auto& child : GetOwner()->GetChildren())
        child->GetTransform()->MarkWorldDirty();
}
