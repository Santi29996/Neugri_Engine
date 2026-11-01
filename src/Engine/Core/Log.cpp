#include "Engine/Core/Log.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace
{
    constexpr std::size_t MAX_ENTRIES = 5000;
    std::deque<LogEntry> entries;

    const char* FileName(const char* path)
    {
        const char* slash = std::strrchr(path, '/');
        const char* backslash = std::strrchr(path, '\\');
        const char* last = slash > backslash ? slash : backslash;
        return last ? last + 1 : path;
    }
}

void Logger::Write(LogLevel level, const char* file, int line, const char* format, ...)
{
    char message[4096];
    va_list args;
    va_start(args, format);
    std::vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // Remove trailing new lines (Assimp messages come with one)
    std::size_t length = std::strlen(message);
    while (length > 0 && (message[length - 1] == '\n' || message[length - 1] == '\r'))
        message[--length] = '\0';

    std::printf("%s(%d) : %s\n", FileName(file), line, message);
    std::fflush(stdout);

    entries.push_back({ level, message });
    if (entries.size() > MAX_ENTRIES)
        entries.pop_front();
}

const std::deque<LogEntry>& Logger::GetEntries()
{
    return entries;
}

void Logger::Clear()
{
    entries.clear();
}
