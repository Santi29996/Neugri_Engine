#pragma once

#include "Engine/Core/Module.h"
#include "Engine/Resources/Texture.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Mesh;

// Initializes the import libraries (DevIL, Assimp logging) and keeps track of every
// mesh and texture alive in the engine so they are only loaded once.
class ModuleResources : public Module
{
public:
    ModuleResources();

    bool Init() override;
    bool Start() override;
    bool CleanUp() override;

    // Textures (DevIL). Returns the cached texture if the file was already loaded.
    std::shared_ptr<Texture> LoadTexture(const std::string& path);
    std::shared_ptr<Texture> LoadTextureFromMemory(const std::string& name, const void* data, std::size_t size);
    std::shared_ptr<Texture> CreateTextureFromPixels(const std::string& name, int width, int height, const unsigned char* rgba);
    const std::shared_ptr<Texture>& GetCheckersTexture() const { return checkersTexture; }

    void RegisterMesh(const std::shared_ptr<Mesh>& mesh) { meshes.push_back(mesh); }

    // Statistics (only resources still in use are counted)
    std::size_t GetLoadedTextureCount() const;
    std::size_t GetLoadedMeshCount() const;
    std::size_t GetTextureMemoryBytes() const;
    std::size_t GetMeshMemoryBytes() const;

    const TextureSettings& GetTextureSettings() const { return textureSettings; }
    void SetTextureSettings(const TextureSettings& settings);

    int GetDevILVersion() const { return devilVersion; }

    static bool IsTextureFile(const std::string& path);
    static bool IsModelFile(const std::string& path);

private:
    std::shared_ptr<Texture> UploadBoundImage(const std::string& name);
    void Purge();

    std::unordered_map<std::string, std::weak_ptr<Texture>> textures;
    std::vector<std::weak_ptr<Mesh>> meshes;
    std::shared_ptr<Texture> checkersTexture;
    TextureSettings textureSettings;
    int devilVersion = 0;
};
