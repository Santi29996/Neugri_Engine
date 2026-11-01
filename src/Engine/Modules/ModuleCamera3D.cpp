#include "Engine/Modules/ModuleCamera3D.h"

#include "Engine/Core/Application.h"
#include "Engine/Modules/ModuleInput.h"
#include "Engine/Modules/ModuleScene.h"
#include "Engine/Modules/ModuleWindow.h"
#include "Engine/Scene/GameObject.h"

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace
{
    constexpr glm::vec3 WORLD_UP(0.0f, 1.0f, 0.0f);
    constexpr float MIN_DISTANCE = 0.05f;
}

ModuleCamera3D::ModuleCamera3D() : Module("Camera3D")
{
    Reset();
}

bool ModuleCamera3D::Start()
{
    // The scene may already have focused the start model, so the camera is not reset here
    return true;
}

void ModuleCamera3D::Reset()
{
    position = { 10.0f, 8.0f, 10.0f };
    LookAt(glm::vec3(0.0f));
}

glm::vec3 ModuleCamera3D::GetForward() const
{
    const float yawRad = glm::radians(yaw);
    const float pitchRad = glm::radians(pitch);
    return glm::normalize(glm::vec3(std::cos(pitchRad) * std::cos(yawRad), std::sin(pitchRad), std::cos(pitchRad) * std::sin(yawRad)));
}

glm::vec3 ModuleCamera3D::GetRight() const
{
    return glm::normalize(glm::cross(GetForward(), WORLD_UP));
}

glm::vec3 ModuleCamera3D::GetUp() const
{
    return glm::normalize(glm::cross(GetRight(), GetForward()));
}

glm::mat4 ModuleCamera3D::GetViewMatrix() const
{
    return glm::lookAt(position, position + GetForward(), WORLD_UP);
}

glm::mat4 ModuleCamera3D::GetProjectionMatrix() const
{
    return glm::perspective(glm::radians(verticalFov), aspectRatio, nearPlane, farPlane);
}

float ModuleCamera3D::ReferenceDistance() const
{
    return std::max(glm::length(reference - position), MIN_DISTANCE);
}

void ModuleCamera3D::LookAt(const glm::vec3& target)
{
    reference = target;
    const glm::vec3 direction = target - position;
    if (glm::length(direction) < 1e-5f)
        return;
    const glm::vec3 d = glm::normalize(direction);
    pitch = glm::degrees(std::asin(std::clamp(d.y, -1.0f, 1.0f)));
    yaw = glm::degrees(std::atan2(d.z, d.x));
}

void ModuleCamera3D::Focus(const AABB& box)
{
    if (!box.IsValid())
        return;
    const float radius = std::max(box.Radius(), 0.1f);
    // Distance so the bounding sphere fits in the vertical field of view
    const float distance = radius / std::sin(glm::radians(verticalFov) * 0.5f) * 1.1f;
    reference = box.Center();
    position = reference - GetForward() * distance;
    farPlane = std::max(farPlane, distance * 10.0f);
}

UpdateStatus ModuleCamera3D::Update(float dt)
{
    const ModuleInput* input = App->input;
    const float dx = input->GetMouseMotionX();
    const float dy = input->GetMouseMotionY();

    // Start interactions only when the mouse is over the scene, keep them while the button is held
    if (mouseInputEnabled)
    {
        if (input->GetMouseButton(SDL_BUTTON_RIGHT) == KeyState::Down)
            looking = true;
        if (input->GetMouseButton(SDL_BUTTON_LEFT) == KeyState::Down && input->IsAltPressed())
            orbiting = true;
        if (input->GetMouseButton(SDL_BUTTON_MIDDLE) == KeyState::Down)
            panning = true;
        if (input->GetMouseWheel() != 0.0f)
            Zoom(input->GetMouseWheel());
    }
    if (!input->IsMouseButtonPressed(SDL_BUTTON_RIGHT)) looking = false;
    if (!input->IsMouseButtonPressed(SDL_BUTTON_LEFT)) orbiting = false;
    if (!input->IsMouseButtonPressed(SDL_BUTTON_MIDDLE)) panning = false;

    // Hide the cursor and get relative motion while looking around
    SDL_Window* window = App->window->GetWindow();
    if (looking != SDL_GetWindowRelativeMouseMode(window))
        SDL_SetWindowRelativeMouseMode(window, looking);

    if (looking)
    {
        UpdateLook(dx, dy);
        Move(dt);
    }
    else if (orbiting)
        Orbit(dx, dy);
    else if (panning)
        Pan(dx, dy);

    if (keyboardInputEnabled && !looking && input->GetKey(SDL_SCANCODE_F) == KeyState::Down)
    {
        if (const GameObject* selected = App->scene->GetSelected())
            Focus(selected->GetHierarchyAABB());
    }
    return UpdateStatus::Continue;
}

void ModuleCamera3D::UpdateLook(float dx, float dy)
{
    const float distance = ReferenceDistance();
    yaw += dx * lookSensitivity;
    pitch = std::clamp(pitch - dy * lookSensitivity, -89.0f, 89.0f);
    reference = position + GetForward() * distance; // keep the orbit pivot in front of the camera
}

void ModuleCamera3D::Move(float dt)
{
    if (!keyboardInputEnabled)
        return;
    const ModuleInput* input = App->input;
    float speed = moveSpeed * dt;
    if (input->IsShiftPressed())
        speed *= 2.0f;

    glm::vec3 movement(0.0f);
    if (input->IsKeyPressed(SDL_SCANCODE_W)) movement += GetForward();
    if (input->IsKeyPressed(SDL_SCANCODE_S)) movement -= GetForward();
    if (input->IsKeyPressed(SDL_SCANCODE_D)) movement += GetRight();
    if (input->IsKeyPressed(SDL_SCANCODE_A)) movement -= GetRight();
    if (input->IsKeyPressed(SDL_SCANCODE_E)) movement += WORLD_UP;
    if (input->IsKeyPressed(SDL_SCANCODE_Q)) movement -= WORLD_UP;

    if (glm::length(movement) > 0.0f)
    {
        const glm::vec3 offset = glm::normalize(movement) * speed;
        position += offset;
        reference += offset;
    }
}

void ModuleCamera3D::Orbit(float dx, float dy)
{
    const float distance = ReferenceDistance();
    yaw += dx * lookSensitivity;
    pitch = std::clamp(pitch - dy * lookSensitivity, -89.0f, 89.0f);
    position = reference - GetForward() * distance;
}

void ModuleCamera3D::Pan(float dx, float dy)
{
    const float distance = ReferenceDistance();
    const float factor = distance * 0.0015f * (App->input->IsShiftPressed() ? 2.0f : 1.0f);
    const glm::vec3 offset = (-GetRight() * dx + GetUp() * dy) * factor;
    position += offset;
    reference += offset;
}

void ModuleCamera3D::Zoom(float wheel)
{
    float distance = ReferenceDistance();
    const float factor = App->input->IsShiftPressed() ? zoomSpeed * 2.0f : zoomSpeed;
    const float newDistance = std::max(distance * (1.0f - wheel * factor), MIN_DISTANCE);
    // When very close keep moving forward pushing the reference too
    if (newDistance <= MIN_DISTANCE && wheel > 0.0f)
        reference += GetForward() * (distance * factor);
    distance = newDistance;
    position = reference - GetForward() * distance;
}
