#pragma once

#include "Engine/Core/Module.h"
#include "Engine/Resources/AABB.h"

#include <glm/glm.hpp>

// Editor camera with Unity-like controls:
//  - Right mouse held: free look + WASD (Q/E down/up), fps-like
//  - Mouse wheel: zoom
//  - Alt + left mouse: orbit around the reference point
//  - Middle mouse: pan
//  - F: focus the selected GameObject
//  - Shift: double movement speed
class ModuleCamera3D : public Module
{
public:
    ModuleCamera3D();

    bool Start() override;
    UpdateStatus Update(float dt) override;

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

    void Focus(const AABB& box);
    void LookAt(const glm::vec3& target);
    void Reset();

    const glm::vec3& GetPosition() const { return position; }
    const glm::vec3& GetReference() const { return reference; }
    glm::vec3 GetForward() const;
    glm::vec3 GetRight() const;
    glm::vec3 GetUp() const;

    void SetAspectRatio(float aspect) { aspectRatio = aspect > 0.0f ? aspect : 1.0f; }

    // The editor decides when the camera may react (mouse over the Scene window, no text field active)
    void SetMouseInputEnabled(bool enabled) { mouseInputEnabled = enabled; }
    void SetKeyboardInputEnabled(bool enabled) { keyboardInputEnabled = enabled; }
    bool IsInteracting() const { return looking || orbiting || panning; }

    float verticalFov = 60.0f; // degrees
    float nearPlane = 0.1f;
    float farPlane = 2000.0f;
    float moveSpeed = 8.0f;
    float lookSensitivity = 0.15f;
    float zoomSpeed = 0.12f;

private:
    void UpdateLook(float dx, float dy);
    void Move(float dt);
    void Orbit(float dx, float dy);
    void Pan(float dx, float dy);
    void Zoom(float wheel);
    float ReferenceDistance() const;

    glm::vec3 position { 10.0f, 8.0f, 10.0f };
    glm::vec3 reference { 0.0f };
    float yaw = -135.0f;  // degrees, around world Y
    float pitch = -30.0f; // degrees
    float aspectRatio = 16.0f / 9.0f;

    bool mouseInputEnabled = true;
    bool keyboardInputEnabled = true;
    bool looking = false;
    bool orbiting = false;
    bool panning = false;
};
