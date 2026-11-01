#pragma once

#include <deque>
#include <string>

enum class LogLevel { Info, Warning, Error };

struct LogEntry
{
    LogLevel level = LogLevel::Info;
    std::string text;
};

// Engine-wide log. Every message goes to stdout and is stored so the editor console can show it.
namespace Logger
{
    void Write(LogLevel level, const char* file, int line, const char* format, ...);
    const std::deque<LogEntry>& GetEntries();
    void Clear();
}

#define LOG(format, ...)       Logger::Write(LogLevel::Info, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) Logger::Write(LogLevel::Warning, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Logger::Write(LogLevel::Error, __FILE__, __LINE__, format, ##__VA_ARGS__)
