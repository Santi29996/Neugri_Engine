#pragma once

#include <memory>

class Mesh;

enum class PrimitiveType { Cube, Sphere, Cylinder, Pyramid, Plane, Torus };

// Procedural basic shapes with normals and texture coordinates
namespace Primitives
{
    std::shared_ptr<Mesh> Create(PrimitiveType type);
    const char* GetName(PrimitiveType type);
}
