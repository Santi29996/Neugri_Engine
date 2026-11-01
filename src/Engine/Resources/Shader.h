#pragma once

#include <glm/glm.hpp>

#include <string>
#include <string_view>
#include <unordered_map>

// Vertex + fragment shader program
class Shader
{
public:
    Shader(std::string name, std::string_view vertexSource, std::string_view fragmentSource);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool IsValid() const { return program != 0; }
    void Use() const;

    void SetInt(const char* uniform, int value) const;
    void SetBool(const char* uniform, bool value) const { SetInt(uniform, value ? 1 : 0); }
    void SetFloat(const char* uniform, float value) const;
    void SetVec3(const char* uniform, const glm::vec3& value) const;
    void SetVec4(const char* uniform, const glm::vec4& value) const;
    void SetMat3(const char* uniform, const glm::mat3& value) const;
    void SetMat4(const char* uniform, const glm::mat4& value) const;

private:
    int Location(const char* uniform) const;

    std::string name;
    unsigned int program = 0;
    mutable std::unordered_map<std::string, int> locations;
};
