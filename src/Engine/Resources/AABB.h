#pragma once

#include <glm/glm.hpp>

#include <array>
#include <limits>

// Axis aligned bounding box
struct AABB
{
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

    bool IsValid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
    glm::vec3 Center() const { return (min + max) * 0.5f; }
    glm::vec3 Size() const { return max - min; }
    float Radius() const { return glm::length(Size()) * 0.5f; }

    void Enclose(const glm::vec3& point)
    {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    void Enclose(const AABB& other)
    {
        if (!other.IsValid())
            return;
        Enclose(other.min);
        Enclose(other.max);
    }

    std::array<glm::vec3, 8> Corners() const
    {
        return { {
            { min.x, min.y, min.z }, { max.x, min.y, min.z }, { min.x, max.y, min.z }, { max.x, max.y, min.z },
            { min.x, min.y, max.z }, { max.x, min.y, max.z }, { min.x, max.y, max.z }, { max.x, max.y, max.z },
        } };
    }

    // Local AABB -> OBB (transformed corners) -> new world AABB
    AABB Transformed(const glm::mat4& transform) const
    {
        AABB result;
        if (!IsValid())
            return result;
        for (const glm::vec3& corner : Corners())
            result.Enclose(glm::vec3(transform * glm::vec4(corner, 1.0f)));
        return result;
    }
};
