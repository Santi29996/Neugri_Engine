#pragma once

#include <string>
#include <utility>

enum class UpdateStatus { Continue, Stop, Error };

// Base class of every engine (and editor) module. The Application calls the
// methods of all modules in the order they were added.
class Module
{
public:
    explicit Module(std::string name) : name(std::move(name)) {}
    virtual ~Module() = default;

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    virtual bool Init() { return true; }
    virtual bool Start() { return true; }
    virtual UpdateStatus PreUpdate(float /*dt*/) { return UpdateStatus::Continue; }
    virtual UpdateStatus Update(float /*dt*/) { return UpdateStatus::Continue; }
    virtual UpdateStatus PostUpdate(float /*dt*/) { return UpdateStatus::Continue; }
    virtual bool CleanUp() { return true; }

    const std::string& GetName() const { return name; }

private:
    std::string name;
};
