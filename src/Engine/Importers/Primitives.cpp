#include "Engine/Importers/Primitives.h"

#include "Engine/Resources/Mesh.h"

#include <glm/gtc/constants.hpp>

#include <cmath>

#include <vector>

namespace
{
    struct Builder
    {
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        std::uint32_t Add(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv)
        {
            vertices.push_back({ position, normal, uv });
            return static_cast<std::uint32_t>(vertices.size() - 1);
        }

        void Triangle(std::uint32_t a, std::uint32_t b, std::uint32_t c) { indices.insert(indices.end(), { a, b, c }); }
        void Quad(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d) { Triangle(a, b, c); Triangle(a, c, d); }

        std::shared_ptr<Mesh> Build(const char* name)
        {
            return std::make_shared<Mesh>(name, std::move(vertices), std::move(indices), true, true);
        }
    };

    // Grid of (rings+1) x (segments+1) vertices produced by a parametric function
    template<typename Function>
    void Parametric(Builder& builder, int segments, int rings, Function function)
    {
        const auto base = static_cast<std::uint32_t>(builder.vertices.size());
        for (int r = 0; r <= rings; ++r)
            for (int s = 0; s <= segments; ++s)
            {
                const float u = static_cast<float>(s) / segments;
                const float v = static_cast<float>(r) / rings;
                glm::vec3 position, normal;
                function(u, v, position, normal);
                builder.Add(position, normal, { u, v });
            }
        const auto stride = static_cast<std::uint32_t>(segments + 1);
        for (int r = 0; r < rings; ++r)
            for (int s = 0; s < segments; ++s)
            {
                const std::uint32_t a = base + r * stride + s;
                builder.Quad(a, a + 1, a + 1 + stride, a + stride);
            }
    }

    std::shared_ptr<Mesh> Cube()
    {
        Builder b;
        const glm::vec3 normals[6] = { { 0, 0, 1 }, { 0, 0, -1 }, { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 } };
        for (const glm::vec3& n : normals)
        {
            // Two axes perpendicular to the face normal
            const glm::vec3 up = glm::abs(n.y) > 0.5f ? glm::vec3(0, 0, -n.y) : glm::vec3(0, 1, 0);
            const glm::vec3 right = glm::cross(up, n);
            const glm::vec3 center = n * 0.5f;
            const std::uint32_t i0 = b.Add(center - right * 0.5f - up * 0.5f, n, { 0, 0 });
            const std::uint32_t i1 = b.Add(center + right * 0.5f - up * 0.5f, n, { 1, 0 });
            const std::uint32_t i2 = b.Add(center + right * 0.5f + up * 0.5f, n, { 1, 1 });
            const std::uint32_t i3 = b.Add(center - right * 0.5f + up * 0.5f, n, { 0, 1 });
            b.Quad(i0, i1, i2, i3);
        }
        return b.Build("Cube");
    }

    std::shared_ptr<Mesh> Sphere()
    {
        Builder b;
        Parametric(b, 32, 16, [](float u, float v, glm::vec3& p, glm::vec3& n) {
            const float theta = u * glm::two_pi<float>();
            const float phi = v * glm::pi<float>();
            n = { -std::sin(phi) * std::cos(theta), -std::cos(phi), std::sin(phi) * std::sin(theta) };
            p = n * 0.5f;
        });
        return b.Build("Sphere");
    }

    std::shared_ptr<Mesh> Cylinder()
    {
        Builder b;
        constexpr int SEGMENTS = 32;
        Parametric(b, SEGMENTS, 1, [](float u, float v, glm::vec3& p, glm::vec3& n) {
            const float theta = u * glm::two_pi<float>();
            n = { std::cos(theta), 0.0f, -std::sin(theta) };
            p = { n.x * 0.5f, v - 0.5f, n.z * 0.5f };
        });
        // Caps
        for (const float y : { 0.5f, -0.5f })
        {
            const glm::vec3 normal(0.0f, y > 0 ? 1.0f : -1.0f, 0.0f);
            const std::uint32_t center = b.Add({ 0.0f, y, 0.0f }, normal, { 0.5f, 0.5f });
            const auto first = static_cast<std::uint32_t>(b.vertices.size());
            for (int s = 0; s <= SEGMENTS; ++s)
            {
                const float theta = static_cast<float>(s) / SEGMENTS * glm::two_pi<float>();
                const float x = std::cos(theta), z = -std::sin(theta);
                b.Add({ x * 0.5f, y, z * 0.5f }, normal, { x * 0.5f + 0.5f, z * 0.5f + 0.5f });
            }
            for (int s = 0; s < SEGMENTS; ++s)
                if (y > 0) b.Triangle(center, first + s, first + s + 1);
                else b.Triangle(center, first + s + 1, first + s);
        }
        return b.Build("Cylinder");
    }

    std::shared_ptr<Mesh> Pyramid()
    {
        Builder b;
        const glm::vec3 apex(0.0f, 0.5f, 0.0f);
        const glm::vec3 base[4] = { { -0.5f, -0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f }, { 0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f } };
        for (int i = 0; i < 4; ++i)
        {
            const glm::vec3& a = base[i];
            const glm::vec3& c = base[(i + 1) % 4];
            const glm::vec3 normal = glm::normalize(glm::cross(c - a, apex - a));
            b.Triangle(b.Add(a, normal, { 0, 0 }), b.Add(c, normal, { 1, 0 }), b.Add(apex, normal, { 0.5f, 1 }));
        }
        const glm::vec3 down(0, -1, 0);
        b.Quad(b.Add(base[3], down, { 0, 0 }), b.Add(base[2], down, { 1, 0 }), b.Add(base[1], down, { 1, 1 }), b.Add(base[0], down, { 0, 1 }));
        return b.Build("Pyramid");
    }

    std::shared_ptr<Mesh> Plane()
    {
        Builder b;
        const glm::vec3 up(0, 1, 0);
        b.Quad(b.Add({ -0.5f, 0, 0.5f }, up, { 0, 0 }), b.Add({ 0.5f, 0, 0.5f }, up, { 1, 0 }),
               b.Add({ 0.5f, 0, -0.5f }, up, { 1, 1 }), b.Add({ -0.5f, 0, -0.5f }, up, { 0, 1 }));
        return b.Build("Plane");
    }

    std::shared_ptr<Mesh> Torus()
    {
        Builder b;
        Parametric(b, 48, 24, [](float u, float v, glm::vec3& p, glm::vec3& n) {
            constexpr float R = 0.35f, r = 0.15f;
            const float theta = u * glm::two_pi<float>();
            const float phi = v * glm::two_pi<float>();
            const glm::vec3 ring(std::cos(theta), 0.0f, -std::sin(theta));
            n = ring * std::cos(phi) + glm::vec3(0.0f, std::sin(phi), 0.0f);
            p = ring * R + n * r;
        });
        return b.Build("Torus");
    }
}

std::shared_ptr<Mesh> Primitives::Create(PrimitiveType type)
{
    switch (type)
    {
    case PrimitiveType::Cube: return Cube();
    case PrimitiveType::Sphere: return Sphere();
    case PrimitiveType::Cylinder: return Cylinder();
    case PrimitiveType::Pyramid: return Pyramid();
    case PrimitiveType::Plane: return Plane();
    case PrimitiveType::Torus: return Torus();
    }
    return nullptr;
}

const char* Primitives::GetName(PrimitiveType type)
{
    switch (type)
    {
    case PrimitiveType::Cube: return "Cube";
    case PrimitiveType::Sphere: return "Sphere";
    case PrimitiveType::Cylinder: return "Cylinder";
    case PrimitiveType::Pyramid: return "Pyramid";
    case PrimitiveType::Plane: return "Plane";
    case PrimitiveType::Torus: return "Torus";
    }
    return "Primitive";
}
