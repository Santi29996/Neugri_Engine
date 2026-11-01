#pragma once

#include <string>
#include <utility>

namespace Editor
{
    // Base class of every dockable editor panel. Each panel can be shown/hidden from the View menu.
    class EditorWindow
    {
    public:
        explicit EditorWindow(std::string name, bool open = true) : name(std::move(name)), open(open) {}
        virtual ~EditorWindow() = default;

        EditorWindow(const EditorWindow&) = delete;
        EditorWindow& operator=(const EditorWindow&) = delete;

        virtual void Draw() = 0;

        const std::string& GetName() const { return name; }
        bool IsOpen() const { return open; }
        void SetOpen(bool value) { open = value; }
        bool* OpenFlag() { return &open; }

    private:
        std::string name;
        bool open = true;
    };
}
