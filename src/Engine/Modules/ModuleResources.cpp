#include "Engine/Modules/ModuleResources.h"

#include "Engine/Core/Log.h"
#include "Engine/Resources/Mesh.h"

#include <IL/il.h>
#include <IL/ilu.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>

namespace
{
    std::string Extension(const std::string& path)
    {
        std::string extension = std::filesystem::path(path).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return extension;
    }

    void AssimpLogCallback(const char* message, char* /*user*/)
    {
        Logger::Write(LogLevel::Info, "Assimp", 0, "[Assimp] %s", message);
    }

    const char* DevILError()
    {
        return iluErrorString(ilGetError());
    }
}

ModuleResources::ModuleResources() : Module("Resources") {}

bool ModuleResources::Init()
{
    // DevIL
    ilInit();
    iluInit();
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT); // match OpenGL texture coordinates
    devilVersion = ilGetInteger(IL_VERSION_NUM);
    LOG("DevIL %d.%d.%d initialized", devilVersion / 100, (devilVersion / 10) % 10, devilVersion % 10);

    // Assimp: stream its log messages to our console
    aiLogStream stream {};
    stream.callback = AssimpLogCallback;
    stream.user = nullptr;
    aiAttachLogStream(&stream);
    LOG("Assimp %u.%u.%u initialized (log stream attached)", aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionPatch());
    return true;
}

bool ModuleResources::Start()
{
    // Procedural black & white checkers texture
    constexpr int SIZE = 64;
    std::array<unsigned char, SIZE * SIZE * 4> pixels {};
    for (int i = 0; i < SIZE; ++i)
    {
        for (int j = 0; j < SIZE; ++j)
        {
            const unsigned char c = (((i & 0x8) == 0) ^ ((j & 0x8) == 0)) ? 255 : 0;
            unsigned char* texel = &pixels[(i * SIZE + j) * 4];
            texel[0] = texel[1] = texel[2] = c;
            texel[3] = 255;
        }
    }
    TextureSettings checkersSettings;
    checkersSettings.linearFilter = false;
    checkersSettings.mipmaps = false;
    checkersTexture = std::make_shared<Texture>("<checkers>", SIZE, SIZE, pixels.data(), checkersSettings);
    LOG("Checkers texture created (%dx%d)", SIZE, SIZE);
    return true;
}

bool ModuleResources::CleanUp()
{
    checkersTexture.reset();
    textures.clear();
    meshes.clear();
    aiDetachAllLogStreams();
    ilShutDown();
    LOG("DevIL shut down, Assimp log streams detached");
    return true;
}

std::shared_ptr<Texture> ModuleResources::LoadTexture(const std::string& path)
{
    Purge();
    const std::string key = std::filesystem::path(path).lexically_normal().generic_string();
    if (const auto it = textures.find(key); it != textures.end())
        if (auto cached = it->second.lock())
            return cached;

    const ILuint image = ilGenImage();
    ilBindImage(image);
    if (!ilLoadImage(path.c_str()))
    {
        LOG_ERROR("DevIL could not load texture %s: %s", path.c_str(), DevILError());
        ilDeleteImage(image);
        return nullptr;
    }
    std::shared_ptr<Texture> texture = UploadBoundImage(key);
    ilDeleteImage(image);
    if (texture)
        textures[key] = texture;
    return texture;
}

std::shared_ptr<Texture> ModuleResources::LoadTextureFromMemory(const std::string& name, const void* data, std::size_t size)
{
    if (const auto it = textures.find(name); it != textures.end())
        if (auto cached = it->second.lock())
            return cached;

    const ILuint image = ilGenImage();
    ilBindImage(image);
    if (!ilLoadL(IL_TYPE_UNKNOWN, data, static_cast<ILuint>(size)))
    {
        LOG_ERROR("DevIL could not load embedded texture %s: %s", name.c_str(), DevILError());
        ilDeleteImage(image);
        return nullptr;
    }
    std::shared_ptr<Texture> texture = UploadBoundImage(name);
    ilDeleteImage(image);
    if (texture)
        textures[name] = texture;
    return texture;
}

std::shared_ptr<Texture> ModuleResources::CreateTextureFromPixels(const std::string& name, int width, int height, const unsigned char* rgba)
{
    auto texture = std::make_shared<Texture>(name, width, height, rgba, textureSettings);
    textures[name] = texture;
    return texture;
}

std::shared_ptr<Texture> ModuleResources::UploadBoundImage(const std::string& name)
{
    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE))
    {
        LOG_ERROR("DevIL could not convert %s to RGBA: %s", name.c_str(), DevILError());
        return nullptr;
    }
    const int width = ilGetInteger(IL_IMAGE_WIDTH);
    const int height = ilGetInteger(IL_IMAGE_HEIGHT);
    auto texture = std::make_shared<Texture>(name, width, height, ilGetData(), textureSettings);
    LOG("DevIL loaded texture %s (%dx%d)", name.c_str(), width, height);
    return texture;
}

void ModuleResources::SetTextureSettings(const TextureSettings& settings)
{
    textureSettings = settings;
    for (const auto& [path, weak] : textures)
        if (const auto texture = weak.lock())
            texture->ApplySettings(textureSettings);
}

void ModuleResources::Purge()
{
    std::erase_if(textures, [](const auto& entry) { return entry.second.expired(); });
    std::erase_if(meshes, [](const std::weak_ptr<Mesh>& mesh) { return mesh.expired(); });
}

std::size_t ModuleResources::GetLoadedTextureCount() const
{
    return static_cast<std::size_t>(std::count_if(textures.begin(), textures.end(), [](const auto& entry) { return !entry.second.expired(); }));
}

std::size_t ModuleResources::GetLoadedMeshCount() const
{
    return static_cast<std::size_t>(std::count_if(meshes.begin(), meshes.end(), [](const auto& mesh) { return !mesh.expired(); }));
}

std::size_t ModuleResources::GetTextureMemoryBytes() const
{
    std::size_t bytes = 0;
    for (const auto& [path, weak] : textures)
        if (const auto texture = weak.lock())
            bytes += texture->GetMemoryBytes();
    return bytes;
}

std::size_t ModuleResources::GetMeshMemoryBytes() const
{
    std::size_t bytes = 0;
    for (const auto& weak : meshes)
        if (const auto mesh = weak.lock())
            bytes += mesh->GetMemoryBytes();
    return bytes;
}

bool ModuleResources::IsTextureFile(const std::string& path)
{
    const std::string extension = Extension(path);
    return extension == ".png" || extension == ".dds" || extension == ".tga" || extension == ".bmp";
}

bool ModuleResources::IsModelFile(const std::string& path)
{
    const std::string extension = Extension(path);
    return extension == ".fbx" || extension == ".obj" || extension == ".gltf" || extension == ".glb";
}
