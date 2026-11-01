#include "Engine/Resources/Shader.h"

#include "Engine/Core/Log.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <array>

namespace
{
    GLuint Compile(GLenum type, std::string_view source, const std::string& name)
    {
        const GLuint shader = glCreateShader(type);
        const char* text = source.data();
        const GLint length = static_cast<GLint>(source.size());
        glShaderSource(shader, 1, &text, &length);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == 0)
        {
            std::array<char, 1024> info {};
            glGetShaderInfoLog(shader, static_cast<GLsizei>(info.size()), nullptr, info.data());
            LOG_ERROR("Shader '%s' (%s) compilation error: %s", name.c_str(), type == GL_VERTEX_SHADER ? "vertex" : "fragment", info.data());
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }
}

Shader::Shader(std::string shaderName, std::string_view vertexSource, std::string_view fragmentSource)
    : name(std::move(shaderName))
{
    const GLuint vertex = Compile(GL_VERTEX_SHADER, vertexSource, name);
    const GLuint fragment = Compile(GL_FRAGMENT_SHADER, fragmentSource, name);
    if (vertex == 0 || fragment == 0)
    {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    // Once linked the shader objects are not needed anymore
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == 0)
    {
        std::array<char, 1024> info {};
        glGetProgramInfoLog(program, static_cast<GLsizei>(info.size()), nullptr, info.data());
        LOG_ERROR("Shader '%s' link error: %s", name.c_str(), info.data());
        glDeleteProgram(program);
        program = 0;
        return;
    }
    LOG("Shader '%s' compiled and linked", name.c_str());
}

Shader::~Shader()
{
    if (program != 0)
        glDeleteProgram(program);
}

void Shader::Use() const
{
    glUseProgram(program);
}

int Shader::Location(const char* uniform) const
{
    const auto it = locations.find(uniform);
    if (it != locations.end())
        return it->second;
    const int location = glGetUniformLocation(program, uniform);
    locations.emplace(uniform, location);
    return location;
}

void Shader::SetInt(const char* uniform, int value) const { glUniform1i(Location(uniform), value); }
void Shader::SetFloat(const char* uniform, float value) const { glUniform1f(Location(uniform), value); }
void Shader::SetVec3(const char* uniform, const glm::vec3& value) const { glUniform3fv(Location(uniform), 1, glm::value_ptr(value)); }
void Shader::SetVec4(const char* uniform, const glm::vec4& value) const { glUniform4fv(Location(uniform), 1, glm::value_ptr(value)); }
void Shader::SetMat3(const char* uniform, const glm::mat3& value) const { glUniformMatrix3fv(Location(uniform), 1, GL_FALSE, glm::value_ptr(value)); }
void Shader::SetMat4(const char* uniform, const glm::mat4& value) const { glUniformMatrix4fv(Location(uniform), 1, GL_FALSE, glm::value_ptr(value)); }
