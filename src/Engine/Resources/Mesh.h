#pragma once

#include "Engine/Resources/AABB.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

struct Vertex
{
    glm::vec3 position { 0.0f };
    glm::vec3 normal { 0.0f, 1.0f, 0.0f };
    glm::vec2 uv { 0.0f };
};

// Geometry stored in VRAM (VAO + interleaved VBO + EBO). A CPU copy is kept for
// bounding boxes, normals debug drawing and (later) mouse picking.
class Mesh
{
public:
    Mesh(std::string name, std::vector<Vertex> vertices, std::vector<std::uint32_t> indices, bool hasNormals, bool hasUVs);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void Draw() const;

    const std::string& GetName() const { return name; }
    const std::string& GetSourcePath() const { return sourcePath; }
    void SetSourcePath(std::string path) { sourcePath = std::move(path); }

    const std::vector<Vertex>& GetVertices() const { return vertices; }
    const std::vector<std::uint32_t>& GetIndices() const { return indices; }
    std::size_t GetTriangleCount() const { return indices.size() / 3; }
    const AABB& GetAABB() const { return aabb; }
    bool HasNormals() const { return hasNormals; }
    bool HasUVs() const { return hasUVs; }
    std::size_t GetMemoryBytes() const;

private:
    std::string name;
    std::string sourcePath;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    AABB aabb;
    bool hasNormals = false;
    bool hasUVs = false;

    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
};
