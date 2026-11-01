#include "Engine/Modules/ModuleInput.h"

#include "Engine/Core/Application.h"
#include "Engine/Core/Log.h"

#include <SDL3/SDL.h>

namespace
{
    KeyState NextState(KeyState current, bool pressed)
    {
        if (pressed)
            return (current == KeyState::Idle || current == KeyState::Up) ? KeyState::Down : KeyState::Repeat;
        return (current == KeyState::Down || current == KeyState::Repeat) ? KeyState::Up : KeyState::Idle;
    }
}

ModuleInput::ModuleInput() : Module("Input") {}

bool ModuleInput::Init()
{
    LOG("Init SDL input event system");
    return true;
}

UpdateStatus ModuleInput::PreUpdate(float /*dt*/)
{
    mouseMotionX = mouseMotionY = mouseWheel = 0.0f;
    keyPressedEvents.fill(false);
    mousePressedEvents.fill(false);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        for (const EventListener& listener : listeners)
            listener(event);

        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            App->RequestQuit();
            break;
        case SDL_EVENT_MOUSE_MOTION:
            mouseMotionX += event.motion.xrel;
            mouseMotionY += event.motion.yrel;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat && event.key.scancode < SDL_SCANCODE_COUNT)
                keyPressedEvents[event.key.scancode] = true;
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button >= 1 && event.button.button <= MAX_MOUSE_BUTTONS)
                mousePressedEvents[event.button.button - 1] = true;
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            mouseWheel += event.wheel.y;
            break;
        case SDL_EVENT_DROP_FILE:
            if (event.drop.data)
            {
                LOG("File dropped: %s", event.drop.data);
                droppedFiles.emplace_back(event.drop.data);
            }
            break;
        default:
            break;
        }
    }

    const bool* keys = SDL_GetKeyboardState(nullptr);
    // A press and release inside the same frame still counts as Down for one frame
    for (int i = 0; i < SDL_SCANCODE_COUNT; ++i)
        keyboard[i] = NextState(keyboard[i], keys[i] || keyPressedEvents[i]);

    const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mouseX, &mouseY);
    for (int i = 0; i < MAX_MOUSE_BUTTONS; ++i)
        mouse[i] = NextState(mouse[i], (buttons & SDL_BUTTON_MASK(i + 1)) != 0 || mousePressedEvents[i]);

    return UpdateStatus::Continue;
}

bool ModuleInput::IsMouseButtonPressed(int button) const
{
    const KeyState state = GetMouseButton(button);
    return state == KeyState::Down || state == KeyState::Repeat;
}

bool ModuleInput::IsAltPressed() const
{
    return IsKeyPressed(SDL_SCANCODE_LALT) || IsKeyPressed(SDL_SCANCODE_RALT);
}

bool ModuleInput::IsShiftPressed() const
{
    return IsKeyPressed(SDL_SCANCODE_LSHIFT) || IsKeyPressed(SDL_SCANCODE_RSHIFT);
}

std::vector<std::string> ModuleInput::TakeDroppedFiles()
{
    std::vector<std::string> files;
    files.swap(droppedFiles);
    return files;
}

bool ModuleInput::CleanUp()
{
    listeners.clear();
    return true;
}
