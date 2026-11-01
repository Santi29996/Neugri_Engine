#include "Engine/Modules/ModuleRenderer3D.h"

#include "Engine/Core/Application.h"
#include "Engine/Core/Log.h"
#include "Engine/Modules/ModuleCamera3D.h"
#include "Engine/Modules/ModuleResources.h"
#include "Engine/Modules/ModuleScene.h"
#include "Engine/Modules/ModuleWindow.h"
#include "Engine/Resources/Framebuffer.h"
#include "Engine/Resources/Mesh.h"
#include "Engine/Resources/Shader.h"
#include "Engine/Resources/Texture.h"
#include "Engine/Scene/ComponentMaterial.h"
#include "Engine/Scene/ComponentMesh.h"
#include "Engine/Scene/ComponentTransform.h"
#include "Engine/Scene/GameObject.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <cstddef>

namespace
{
    constexpr const char* MESH_VERTEX_SHADER = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

out vec3 vNormal;
out vec2 vTexCoord;

void main()
{
    vNormal = normalMatrix * aNormal;
    vTexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
)";

    constexpr const char* MESH_FRAGMENT_SHADER = R"(#version 330 core
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D diffuseMap;
uniform bool useTexture;
uniform bool useLighting;
uniform vec4 color;
uniform vec3 lightDirection;

out vec4 fragColor;

void main()
{
    vec4 albedo = color;
    if (useTexture)
        albedo *= texture(diffuseMap, vTexCoord);

    if (useLighting)
    {
        vec3 normal = normalize(vNormal);
        if (!gl_FrontFacing)
            normal = -normal;
        float diffuse = max(dot(normal, -normalize(lightDirection)), 0.0);
        fragColor = vec4(albedo.rgb * (0.35 + 0.65 * diffuse), albedo.a);
    }
    else
        fragColor = albedo;
}
)";

    constexpr const char* LINE_VERTEX_SHADER = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
uniform mat4 viewProjection;
out vec3 vColor;
void main()
{
    vColor = aColor;
    gl_Position = viewProjection * vec4(aPosition, 1.0);
}
)";

    constexpr const char* LINE_FRAGMENT_SHADER = R"(#version 330 core
in vec3 vColor;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, 1.0); }
)";

#ifndef NDEBUG
    void GLAPIENTRY OpenGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* message, const void*)
    {
        if (severity != GL_DEBUG_SEVERITY_HIGH && severity != GL_DEBUG_SEVERITY_MEDIUM)
            return;
        const char* sourceName = source == GL_DEBUG_SOURCE_API ? "API" : source == GL_DEBUG_SOURCE_SHADER_COMPILER ? "Shader Compiler" : "Other";
        const char* typeName = type == GL_DEBUG_TYPE_ERROR ? "Error" : type == GL_DEBUG_TYPE_PERFORMANCE ? "Performance" : "Other";
        LOG_WARNING("<OpenGL Source:%s> <Type:%s> <ID:%u> %s", sourceName, typeName, id, message);
    }
#endif
}

ModuleRenderer3D::ModuleRenderer3D() : Module("Renderer3D") {}

ModuleRenderer3D::~ModuleRenderer3D() = default;

bool ModuleRenderer3D::Init()
{
#ifndef NDEBUG
    if (GLAD_GL_KHR_debug || GLAD_GL_VERSION_4_3)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OpenGLDebugCallback, nullptr);
        LOG("OpenGL debug output enabled");
    }
#endif

    meshShader = std::make_unique<Shader>("Default mesh", MESH_VERTEX_SHADER, MESH_FRAGMENT_SHADER);
    lineShader = std::make_unique<Shader>("Debug lines", LINE_VERTEX_SHADER, LINE_FRAGMENT_SHADER);
    if (!meshShader->IsValid() || !lineShader->IsValid())
        return false;

    framebuffer = std::make_unique<Framebuffer>();

    glGenVertexArrays(1, &linesVao);
    glGenBuffers(1, &linesVbo);
    glBindVertexArray(linesVao);
    glBindBuffer(GL_ARRAY_BUFFER, linesVbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), reinterpret_cast<void*>(offsetof(LineVertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), reinterpret_cast<void*>(offsetof(LineVertex, color)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    CreateGrid();
    LOG("Renderer initialized (shader pipeline, OpenGL %d.%d)", App->window->GetGLMajor(), App->window->GetGLMinor());
    return true;
}

bool ModuleRenderer3D::Start()
{
    SDL_GetWindowSizeInPixels(App->window->GetWindow(), &viewportWidth, &viewportHeight);
    return true;
}

void ModuleRenderer3D::CreateGrid()
{
    std::vector<LineVertex> grid;
    constexpr int HALF = 50;
    const glm::vec3 minorColor(0.30f), majorColor(0.42f);
    for (int i = -HALF; i <= HALF; ++i)
    {
        const float f = static_cast<float>(i);
        const glm::vec3 xColor = i == 0 ? glm::vec3(0.25f, 0.35f, 0.85f) : (i % 10 == 0 ? majorColor : minorColor); // Z axis line
        const glm::vec3 zColor = i == 0 ? glm::vec3(0.85f, 0.25f, 0.25f) : (i % 10 == 0 ? majorColor : minorColor); // X axis line
        grid.push_back({ { f, 0.0f, -HALF }, xColor });
        grid.push_back({ { f, 0.0f, HALF }, xColor });
        grid.push_back({ { -HALF, 0.0f, f }, zColor });
        grid.push_back({ { HALF, 0.0f, f }, zColor });
    }
    grid.push_back({ { 0.0f, 0.0f, 0.0f }, { 0.25f, 0.85f, 0.25f } }); // Y axis
    grid.push_back({ { 0.0f, 2.0f, 0.0f }, { 0.25f, 0.85f, 0.25f } });
    gridVertexCount = static_cast<int>(grid.size());

    glGenVertexArrays(1, &gridVao);
    glGenBuffers(1, &gridVbo);
    glBindVertexArray(gridVao);
    glBindBuffer(GL_ARRAY_BUFFER, gridVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(grid.size() * sizeof(LineVertex)), grid.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), reinterpret_cast<void*>(offsetof(LineVertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), reinterpret_cast<void*>(offsetof(LineVertex, color)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void ModuleRenderer3D::SetViewportSize(int width, int height)
{
    if (width > 0 && height > 0)
    {
        viewportWidth = width;
        viewportHeight = height;
    }
}

UpdateStatus ModuleRenderer3D::PreUpdate(float /*dt*/)
{
    if (!renderToFramebuffer) // game mode: render to the whole window
        SDL_GetWindowSizeInPixels(App->window->GetWindow(), &viewportWidth, &viewportHeight);
    App->camera->SetAspectRatio(static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight));
    return UpdateStatus::Continue;
}

void ModuleRenderer3D::ApplyState() const
{
    settings.depthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    settings.cullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glPolygonMode(GL_FRONT_AND_BACK, settings.wireframe ? GL_LINE : GL_FILL);
    glDisable(GL_BLEND);
}

UpdateStatus ModuleRenderer3D::PostUpdate(float /*dt*/)
{
    stats = {};

    if (renderToFramebuffer)
    {
        framebuffer->Resize(viewportWidth, viewportHeight);
        framebuffer->Bind();
    }
    glViewport(0, 0, viewportWidth, viewportHeight);
    glClearColor(settings.backgroundColor.r, settings.backgroundColor.g, settings.backgroundColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    const ModuleCamera3D* camera = App->camera;
    const glm::mat4 view = camera->GetViewMatrix();
    const glm::mat4 projection = camera->GetProjectionMatrix();
    const glm::mat4 viewProjection = projection * view;

    ApplyState();

    // 1. Level geometry
    meshShader->Use();
    meshShader->SetMat4("view", view);
    meshShader->SetMat4("projection", projection);
    meshShader->SetInt("diffuseMap", 0);
    meshShader->SetBool("useLighting", settings.lighting);
    meshShader->SetVec3("lightDirection", settings.lightDirection);
    if (const GameObject* root = App->scene->GetRoot())
        DrawGameObject(*root, viewProjection);
    Texture::Unbind();

    // 2. Debug geometry (grid, bounding boxes, normals)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (settings.showGrid)
        DrawGrid(viewProjection);
    if (settings.showSelectionAABB)
        if (const GameObject* selected = App->scene->GetSelected())
            AddAABB(selected->GetHierarchyAABB(), { 1.0f, 0.6f, 0.1f });
    FlushLines(viewProjection);

    glUseProgram(0);
    if (renderToFramebuffer)
        Framebuffer::Unbind();
    // 3. The editor (if any) draws after this module. 4. The Application swaps the buffers.
    return UpdateStatus::Continue;
}

void ModuleRenderer3D::DrawGameObject(const GameObject& gameObject, const glm::mat4& viewProjection)
{
    if (!gameObject.IsActive())
        return;

    const ComponentMesh* meshComponent = gameObject.GetComponent<ComponentMesh>();
    if (meshComponent && meshComponent->IsActive() && meshComponent->GetMesh())
    {
        const glm::mat4& model = gameObject.GetTransform()->GetWorldMatrix();
        meshShader->SetMat4("model", model);
        meshShader->SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));

        const ComponentMaterial* material = gameObject.GetComponent<ComponentMaterial>();
        const bool materialActive = material && material->IsActive();
        const Texture* texture = nullptr;
        if (settings.textures && materialActive)
        {
            if (material->showCheckers || settings.checkersEverywhere)
                texture = App->resources->GetCheckersTexture().get();
            else
                texture = material->GetTexture().get();
        }
        else if (settings.textures && settings.checkersEverywhere)
            texture = App->resources->GetCheckersTexture().get();

        meshShader->SetVec4("color", materialActive && !(material->showCheckers || settings.checkersEverywhere) ? material->color : glm::vec4(1.0f));
        meshShader->SetBool("useTexture", texture != nullptr);
        if (texture)
            texture->Bind(0);
        else
            Texture::Unbind(0);

        meshComponent->GetMesh()->Draw();
        ++stats.drawCalls;
        stats.triangles += meshComponent->GetMesh()->GetTriangleCount();

        if (meshComponent->showVertexNormals || meshComponent->showFaceNormals)
            DrawNormals(gameObject);
        if (settings.showAllAABB)
            AddAABB(meshComponent->GetWorldAABB(), { 0.2f, 0.9f, 0.4f });
    }

    for (const auto& child : gameObject.GetChildren())
        DrawGameObject(*child, viewProjection);
}

void ModuleRenderer3D::DrawNormals(const GameObject& gameObject)
{
    const ComponentMesh* meshComponent = gameObject.GetComponent<ComponentMesh>();
    const Mesh& mesh = *meshComponent->GetMesh();
    const glm::mat4& model = gameObject.GetTransform()->GetWorldMatrix();
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    const float length = settings.normalsLength;
    const auto& vertices = mesh.GetVertices();
    const auto& indices = mesh.GetIndices();

    if (meshComponent->showVertexNormals && mesh.HasNormals())
        for (const Vertex& vertex : vertices)
        {
            const glm::vec3 start = glm::vec3(model * glm::vec4(vertex.position, 1.0f));
            AddLine(start, start + glm::normalize(normalMatrix * vertex.normal) * length, { 0.2f, 0.8f, 1.0f });
        }

    if (meshComponent->showFaceNormals)
        for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const glm::vec3 a = glm::vec3(model * glm::vec4(vertices[indices[i]].position, 1.0f));
            const glm::vec3 b = glm::vec3(model * glm::vec4(vertices[indices[i + 1]].position, 1.0f));
            const glm::vec3 c = glm::vec3(model * glm::vec4(vertices[indices[i + 2]].position, 1.0f));
            const glm::vec3 normal = glm::cross(b - a, c - a);
            if (glm::length(normal) < 1e-12f)
                continue;
            const glm::vec3 center = (a + b + c) / 3.0f;
            AddLine(center, center + glm::normalize(normal) * length, { 1.0f, 0.3f, 0.8f });
        }
}

void ModuleRenderer3D::DrawGrid(const glm::mat4& viewProjection) const
{
    lineShader->Use();
    lineShader->SetMat4("viewProjection", viewProjection);
    glBindVertexArray(gridVao);
    glDrawArrays(GL_LINES, 0, gridVertexCount);
    glBindVertexArray(0);
}

void ModuleRenderer3D::AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
{
    lines.push_back({ from, color });
    lines.push_back({ to, color });
}

void ModuleRenderer3D::AddAABB(const AABB& box, const glm::vec3& color)
{
    if (!box.IsValid())
        return;
    const auto c = box.Corners();
    constexpr int edges[12][2] = { { 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 }, { 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
    for (const auto& edge : edges)
        AddLine(c[edge[0]], c[edge[1]], color);
}

void ModuleRenderer3D::FlushLines(const glm::mat4& viewProjection)
{
    stats.debugLines = lines.size() / 2;
    if (lines.empty())
        return;
    lineShader->Use();
    lineShader->SetMat4("viewProjection", viewProjection);
    glBindVertexArray(linesVao);
    glBindBuffer(GL_ARRAY_BUFFER, linesVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(lines.size() * sizeof(LineVertex)), lines.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));
    glBindVertexArray(0);
    lines.clear();
}

bool ModuleRenderer3D::CleanUp()
{
    glDeleteBuffers(1, &gridVbo);
    glDeleteVertexArrays(1, &gridVao);
    glDeleteBuffers(1, &linesVbo);
    glDeleteVertexArrays(1, &linesVao);
    meshShader.reset();
    lineShader.reset();
    framebuffer.reset();
    LOG("Renderer cleaned up");
    return true;
}
