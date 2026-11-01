#include "Engine/Importers/ModelImporter.h"

#include "Engine/Core/Application.h"
#include "Engine/Core/Log.h"
#include "Engine/Modules/ModuleResources.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Texture.h"
#include "Engine/Scene/ComponentMaterial.h"
#include "Engine/Scene/ComponentMesh.h"
#include "Engine/Scene/ComponentTransform.h"
#include "Engine/Scene/GameObject.h"

#include <assimp/cimport.h>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <SDL3/SDL_timer.h>

#include <cmath>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    struct ImportContext
    {
        const aiScene* scene = nullptr;
        fs::path directory;
        std::string sourcePath;
        std::vector<std::shared_ptr<Mesh>> meshes;          // indexed like aiScene::mMeshes
        std::vector<std::shared_ptr<Texture>> materials;    // diffuse texture per aiScene::mMaterials
        std::size_t meshObjects = 0;
        bool fixBlenderAxis = false;
        bool blenderAxisFixed = false;
    };

    ModelImporter::Settings settings;

    bool IsBlenderAxisRotation(const glm::quat& rotation)
    {
        const glm::quat blenderAxis = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        return std::abs(glm::dot(rotation, blenderAxis)) > 0.9999f;
    }

    std::shared_ptr<Mesh> ConvertMesh(const aiMesh* source, const std::string& sourcePath)
    {
        std::vector<Vertex> vertices(source->mNumVertices);
        const bool hasNormals = source->HasNormals();
        const bool hasUVs = source->HasTextureCoords(0);

        for (unsigned int i = 0; i < source->mNumVertices; ++i)
        {
            Vertex& vertex = vertices[i];
            vertex.position = { source->mVertices[i].x, source->mVertices[i].y, source->mVertices[i].z };
            if (hasNormals)
                vertex.normal = { source->mNormals[i].x, source->mNormals[i].y, source->mNormals[i].z };
            if (hasUVs)
                vertex.uv = { source->mTextureCoords[0][i].x, source->mTextureCoords[0][i].y };
        }

        std::vector<std::uint32_t> indices;
        indices.reserve(static_cast<std::size_t>(source->mNumFaces) * 3);
        for (unsigned int i = 0; i < source->mNumFaces; ++i)
        {
            const aiFace& face = source->mFaces[i];
            if (face.mNumIndices != 3)
            {
                LOG_WARNING("Geometry face with %u indices skipped (only triangles are supported)", face.mNumIndices);
                continue;
            }
            indices.insert(indices.end(), face.mIndices, face.mIndices + 3);
        }

        const std::string name = source->mName.length > 0 ? source->mName.C_Str() : "Mesh";
        auto mesh = std::make_shared<Mesh>(name, std::move(vertices), std::move(indices), hasNormals, hasUVs);
        mesh->SetSourcePath(sourcePath);
        LOG("New mesh '%s' with %u vertices, %zu triangles%s%s", name.c_str(), source->mNumVertices, mesh->GetTriangleCount(),
            hasNormals ? ", normals" : "", hasUVs ? ", uvs" : "");
        return mesh;
    }

    std::shared_ptr<Texture> LoadMaterialTexture(const ImportContext& context, const aiMaterial* material)
    {
        aiString texturePath;
        bool found = material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS;
        if (!found)
            found = material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS;
        if (!found || texturePath.length == 0)
            return nullptr;

        ModuleResources* resources = App->resources;

        // Texture embedded inside the model file
        if (const aiTexture* embedded = context.scene->GetEmbeddedTexture(texturePath.C_Str()))
        {
            const std::string name = context.sourcePath + "/" + texturePath.C_Str();
            if (embedded->mHeight == 0) // compressed (png, jpg...)
                return resources->LoadTextureFromMemory(name, embedded->pcData, embedded->mWidth);

            // Raw ARGB8888 texels, top-down
            const unsigned width = embedded->mWidth, height = embedded->mHeight;
            std::vector<unsigned char> rgba(static_cast<std::size_t>(width) * height * 4);
            for (unsigned y = 0; y < height; ++y)
                for (unsigned x = 0; x < width; ++x)
                {
                    const aiTexel& texel = embedded->pcData[(height - 1 - y) * width + x];
                    unsigned char* out = &rgba[(static_cast<std::size_t>(y) * width + x) * 4];
                    out[0] = texel.r; out[1] = texel.g; out[2] = texel.b; out[3] = texel.a;
                }
            return resources->CreateTextureFromPixels(name, static_cast<int>(width), static_cast<int>(height), rgba.data());
        }

        // External file: try the stored path and then a few common places
        std::string relative = texturePath.C_Str();
        for (char& c : relative)
            if (c == '\\') c = '/';
        const fs::path stored(relative);
        const fs::path fileName = stored.filename();
        const fs::path candidates[] = {
            stored.is_absolute() ? stored : context.directory / stored,
            context.directory / fileName,
            context.directory / "Textures" / fileName,
            context.directory / "textures" / fileName,
            fs::path("Assets") / fileName,
            fs::path("Assets") / "Textures" / fileName,
        };
        std::error_code error;
        for (const fs::path& candidate : candidates)
        {
            if (fs::exists(candidate, error))
                return resources->LoadTexture(candidate.string());
        }
        LOG_WARNING("Texture '%s' referenced by the model was not found", texturePath.C_Str());
        return nullptr;
    }

    GameObject* CreateMeshObject(ImportContext& context, GameObject* target, unsigned int meshIndex)
    {
        const aiMesh* source = context.scene->mMeshes[meshIndex];
        std::shared_ptr<Mesh>& mesh = context.meshes[meshIndex];
        if (!mesh)
        {
            mesh = ConvertMesh(source, context.sourcePath);
            App->resources->RegisterMesh(mesh);
        }

        target->AddComponent<ComponentMesh>()->SetMesh(mesh);
        ComponentMaterial* material = target->AddComponent<ComponentMaterial>();
        if (source->mMaterialIndex < context.materials.size())
            material->SetTexture(context.materials[source->mMaterialIndex]);

        if (source->mMaterialIndex < context.scene->mNumMaterials)
        {
            aiColor4D diffuse;
            if (aiGetMaterialColor(context.scene->mMaterials[source->mMaterialIndex], AI_MATKEY_COLOR_DIFFUSE, &diffuse) == AI_SUCCESS && !material->GetTexture())
                material->color = { diffuse.r, diffuse.g, diffuse.b, 1.0f };
        }
        ++context.meshObjects;
        return target;
    }

    std::unique_ptr<GameObject> ConvertNode(ImportContext& context, const aiNode* node, int depth)
    {
        auto gameObject = std::make_unique<GameObject>(node->mName.length > 0 ? node->mName.C_Str() : "Node");

        aiVector3D scaling, translation;
        aiQuaternion rotation;
        node->mTransformation.Decompose(scaling, rotation, translation);
        glm::quat localRotation(rotation.w, rotation.x, rotation.y, rotation.z);
        if (context.fixBlenderAxis && depth == 1 && IsBlenderAxisRotation(localRotation))
        {
            localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            context.blenderAxisFixed = true;
        }
        gameObject->GetTransform()->SetLocal({ translation.x, translation.y, translation.z }, localRotation, { scaling.x, scaling.y, scaling.z });

        // One GameObject per mesh: a node with a single mesh holds it, otherwise one child per mesh
        if (node->mNumMeshes == 1)
            CreateMeshObject(context, gameObject.get(), node->mMeshes[0]);
        else
            for (unsigned int i = 0; i < node->mNumMeshes; ++i)
            {
                const aiMesh* source = context.scene->mMeshes[node->mMeshes[i]];
                auto child = std::make_unique<GameObject>(source->mName.length > 0 ? source->mName.C_Str() : gameObject->GetName() + "_mesh" + std::to_string(i));
                CreateMeshObject(context, child.get(), node->mMeshes[i]);
                gameObject->AddChild(std::move(child));
            }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
            gameObject->AddChild(ConvertNode(context, node->mChildren[i], depth + 1));
        return gameObject;
    }
}

ModelImporter::Settings& ModelImporter::GetSettings()
{
    return settings;
}

std::unique_ptr<GameObject> ModelImporter::Import(const std::string& path)
{
    const Uint64 start = SDL_GetTicksNS();
    LOG("Importing model %s", path.c_str());

    aiPropertyStore* properties = aiCreatePropertyStore();
    aiSetImportPropertyInteger(properties, AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, 0); // no $AssimpFbx$ helper nodes
    aiSetImportPropertyInteger(properties, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
    const aiScene* scene = aiImportFileExWithProperties(path.c_str(),
        aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_Triangulate | aiProcess_GenSmoothNormals,
        nullptr, properties);
    aiReleasePropertyStore(properties);

    if (scene == nullptr || scene->mRootNode == nullptr || !scene->HasMeshes())
    {
        LOG_ERROR("Error loading model %s: %s", path.c_str(), aiGetErrorString());
        if (scene)
            aiReleaseImport(scene);
        return nullptr;
    }

    ImportContext context;
    context.scene = scene;
    context.sourcePath = fs::path(path).lexically_normal().generic_string();
    context.directory = fs::path(path).parent_path();
    context.meshes.resize(scene->mNumMeshes);
    context.materials.resize(scene->mNumMaterials);
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        context.materials[i] = LoadMaterialTexture(context, scene->mMaterials[i]);

    // Information about the file that created the model
    std::string generator;
    double unitScaleFactor = 1.0;
    bool hasUnitScale = false;
    if (const aiMetadata* metadata = scene->mMetaData)
    {
        aiString value;
        if (metadata->Get("SourceAsset_Generator", value))
            generator = value.C_Str();
        float unitFloat = 0.0f;
        if (metadata->Get("UnitScaleFactor", unitScaleFactor))
            hasUnitScale = true;
        else if (metadata->Get("UnitScaleFactor", unitFloat))
        {
            unitScaleFactor = unitFloat;
            hasUnitScale = true;
        }
    }
    if (!generator.empty())
        LOG("Model created with: %s", generator.c_str());
    context.fixBlenderAxis = settings.fixBlenderAxis && generator.find("Blender") != std::string::npos;

    std::unique_ptr<GameObject> root = ConvertNode(context, scene->mRootNode, 0);
    root->SetName(fs::path(path).stem().string());

    if (context.blenderAxisFixed)
        LOG_WARNING("Blender -90 deg X axis rotation removed from the root objects (Configuration > Import to disable)");
    if (settings.convertUnitsToMeters && hasUnitScale && unitScaleFactor > 0.0)
    {
        const float toMeters = static_cast<float>(unitScaleFactor * 0.01); // UnitScaleFactor is in centimeters
        if (std::abs(toMeters - 1.0f) > 1e-6f)
        {
            root->GetTransform()->SetScale(root->GetTransform()->GetScale() * toMeters);
            LOG("FBX units converted to meters (scale %.4f)", toMeters);
        }
    }

    aiReleaseImport(scene);

    const double ms = static_cast<double>(SDL_GetTicksNS() - start) / 1e6;
    LOG("Model %s imported in %.2f ms: %zu mesh GameObjects, %zu meshes, %u materials",
        root->GetName().c_str(), ms, context.meshObjects, context.meshes.size(), static_cast<unsigned>(context.materials.size()));
    return root;
}
