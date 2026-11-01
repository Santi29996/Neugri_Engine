#pragma once

#include "Engine/Core/Module.h"
#include "Engine/Resources/AABB.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class Framebuffer;
class GameObject;
class Shader;

struct RenderSettings
{
    bool depthTest = true;
    bool cullFace = true;
    bool lighting = true;
    bool textures = true;
    bool wireframe = false;
    bool showGrid = true;
    bool showSelectionAABB = true;
    bool showAllAABB = false;
    bool checkersEverywhere = false;
    float normalsLength = 0.2f;
    glm::vec3 backgroundColor { 0.16f, 0.17f, 0.19f };
    glm::vec3 lightDirection { -0.4f, -1.0f, -0.6f };
};

struct RenderStats
{
    int drawCalls = 0;
    std::size_t triangles = 0;
    std::size_t debugLines = 0;
};

// Draws the scene with a simple shader pipeline. When the editor is present the
// scene is rendered into a framebuffer that the Scene window displays.
class ModuleRenderer3D : public Module
{
public:
    ModuleRenderer3D();
    ~ModuleRenderer3D() override;

    bool Init() override;
    bool Start() override;
    UpdateStatus PreUpdate(float dt) override;
    UpdateStatus PostUpdate(float dt) override;
    bool CleanUp() override;

    void SetRenderToFramebuffer(bool enabled) { renderToFramebuffer = enabled; }
    void SetViewportSize(int width, int height);
    const Framebuffer* GetFramebuffer() const { return framebuffer.get(); }

    // Debug lines are collected during the frame and drawn on top of the geometry
    void AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);
    void AddAABB(const AABB& box, const glm::vec3& color);

    RenderSettings settings;
    const RenderStats& GetStats() const { return stats; }

private:
    struct LineVertex
    {
        glm::vec3 position;
        glm::vec3 color;
    };

    void ApplyState() const;
    void DrawGameObject(const GameObject& gameObject, const glm::mat4& viewProjection);
    void DrawNormals(const GameObject& gameObject);
    void DrawGrid(const glm::mat4& viewProjection) const;
    void FlushLines(const glm::mat4& viewProjection);
    void CreateGrid();

    std::unique_ptr<Framebuffer> framebuffer;
    std::unique_ptr<Shader> meshShader;
    std::unique_ptr<Shader> lineShader;
    bool renderToFramebuffer = false;
    int viewportWidth = 1280;
    int viewportHeight = 720;

    unsigned int gridVao = 0, gridVbo = 0;
    int gridVertexCount = 0;
    unsigned int linesVao = 0, linesVbo = 0;
    std::vector<LineVertex> lines;

    RenderStats stats;
};
