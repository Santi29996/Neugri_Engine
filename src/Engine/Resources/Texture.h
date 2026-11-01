#pragma once

#include <string>

struct TextureSettings
{
    bool linearFilter = true;
    bool mipmaps = true;
    bool repeat = true;
};

// Texture stored in VRAM
class Texture
{
public:
    // pixels must be RGBA8, rows from bottom to top (OpenGL convention)
    Texture(std::string path, int width, int height, const unsigned char* pixels, const TextureSettings& settings);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void Bind(unsigned int unit = 0) const;
    static void Unbind(unsigned int unit = 0);
    void ApplySettings(const TextureSettings& settings) const;

    unsigned int GetId() const { return id; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    const std::string& GetPath() const { return path; }
    std::size_t GetMemoryBytes() const { return static_cast<std::size_t>(width) * height * 4; }

private:
    std::string path;
    int width = 0;
    int height = 0;
    unsigned int id = 0;
};
