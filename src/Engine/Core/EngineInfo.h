#pragma once

#include <array>
#include <string_view>

// Single place to edit the engine identity (window title, About window, Help menu links)
namespace EngineInfo
{
    constexpr std::string_view NAME = "Neugri Engine";
    constexpr std::string_view VERSION = ENGINE_VERSION_STRING;

    constexpr std::string_view REPOSITORY_URL = "https://github.com/Santi29996/Repositorio-Personal";

    struct TeamMember
    {
        std::string_view name;
        std::string_view github;
    };

    constexpr std::array<TeamMember, 1> TEAM = { {
        { "Santiago \u00C1lvarez Cervera", "github.com/Santi29996" },
    } };
}
