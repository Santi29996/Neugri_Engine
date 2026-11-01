#pragma once

#include "Engine/Core/Module.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>

#include <array>
#include <functional>
#include <string>
#include <vector>

enum class KeyState { Idle, Down, Repeat, Up };

class ModuleInput : public Module
{
public:
    static constexpr int MAX_MOUSE_BUTTONS = 5;
    using EventListener = std::function<void(const SDL_Event&)>;

    ModuleInput();

    bool Init() override;
    UpdateStatus PreUpdate(float dt) override;
    bool CleanUp() override;

    KeyState GetKey(SDL_Scancode key) const { return keyboard[key]; }
    bool IsKeyPressed(SDL_Scancode key) const { return keyboard[key] == KeyState::Down || keyboard[key] == KeyState::Repeat; }
    KeyState GetMouseButton(int button) const { return mouse[button - 1]; } // SDL_BUTTON_LEFT == 1
    bool IsMouseButtonPressed(int button) const;

    float GetMouseX() const { return mouseX; }
    float GetMouseY() const { return mouseY; }
    float GetMouseMotionX() const { return mouseMotionX; }
    float GetMouseMotionY() const { return mouseMotionY; }
    float GetMouseWheel() const { return mouseWheel; }

    bool IsAltPressed() const;
    bool IsShiftPressed() const;

    // Files dropped into the window (or passed through the command line) during this frame
    void QueueDroppedFile(std::string path) { droppedFiles.push_back(std::move(path)); }
    std::vector<std::string> TakeDroppedFiles();

    // The editor registers here to receive raw SDL events (ImGui)
    void AddEventListener(EventListener listener) { listeners.push_back(std::move(listener)); }

private:
    std::array<KeyState, SDL_SCANCODE_COUNT> keyboard {};
    std::array<KeyState, MAX_MOUSE_BUTTONS> mouse {};
    std::array<bool, SDL_SCANCODE_COUNT> keyPressedEvents {};
    std::array<bool, MAX_MOUSE_BUTTONS> mousePressedEvents {};
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float mouseMotionX = 0.0f;
    float mouseMotionY = 0.0f;
    float mouseWheel = 0.0f;

    std::vector<std::string> droppedFiles;
    std::vector<EventListener> listeners;
};
