#include "Editor/Windows/WindowConsole.h"

#include "Engine/Core/Log.h"

#include <algorithm>

namespace Editor
{
    void WindowConsole::Draw()
    {
        if (!ImGui::Begin(GetName().c_str(), OpenFlag()))
        {
            ImGui::End();
            return;
        }

        const auto& entries = Logger::GetEntries();
        const auto countLevel = [&](LogLevel level) {
            return std::count_if(entries.begin(), entries.end(), [level](const LogEntry& entry) { return entry.level == level; });
        };

        if (ImGui::Button("Clear"))
            Logger::Clear();
        ImGui::SameLine();
        if (ImGui::Button("Copy"))
        {
            ImGui::LogToClipboard();
            for (const LogEntry& entry : entries)
                ImGui::LogText("%s\n", entry.text.c_str());
            ImGui::LogFinish();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &autoScroll);
        ImGui::SameLine();
        ImGui::Checkbox(("Info (" + std::to_string(countLevel(LogLevel::Info)) + ")").c_str(), &showInfo);
        ImGui::SameLine();
        ImGui::Checkbox(("Warnings (" + std::to_string(countLevel(LogLevel::Warning)) + ")").c_str(), &showWarnings);
        ImGui::SameLine();
        ImGui::Checkbox(("Errors (" + std::to_string(countLevel(LogLevel::Error)) + ")").c_str(), &showErrors);
        ImGui::SameLine();
        filter.Draw("Filter", 180.0f);
        ImGui::Separator();

        if (ImGui::BeginChild("ConsoleScroll", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            for (const LogEntry& entry : entries)
            {
                if ((entry.level == LogLevel::Info && !showInfo) || (entry.level == LogLevel::Warning && !showWarnings) ||
                    (entry.level == LogLevel::Error && !showErrors) || !filter.PassFilter(entry.text.c_str()))
                    continue;

                ImVec4 color(0.85f, 0.85f, 0.85f, 1.0f);
                if (entry.level == LogLevel::Warning) color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
                else if (entry.level == LogLevel::Error) color = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
                else if (entry.text.rfind("[Assimp]", 0) == 0) color = ImVec4(0.6f, 0.75f, 1.0f, 1.0f);
                ImGui::TextColored(color, "%s", entry.text.c_str());
            }
            if (autoScroll && entries.size() != lastCount)
                ImGui::SetScrollHereY(1.0f);
            lastCount = entries.size();
        }
        ImGui::EndChild();
        ImGui::End();
    }
}
