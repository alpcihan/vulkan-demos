#pragma once

#include "gpro/gpro.hpp"

namespace gpro
{

typedef uint32_t IndexFormat;

struct Transform {
    Transform(glm::vec3 translation, glm::vec3 rotation = glm::vec3(0.0), glm::vec3 scale = glm::vec3(1.0))
    {
        transform = glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(glm::quat(glm::radians(rotation))) *
                    glm::scale(glm::mat4(1.0f), scale);
    }

    alignas(16) glm::mat4 transform = glm::mat4(1);
};

struct Light {
    alignas(16) glm::vec3 lightPos = glm::vec3(0);
    alignas(16) glm::vec3 lightColor = glm::vec3(1);
};

struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    // glm::vec3 tangent;
    // glm::vec3 bitangent;

    bool operator==(const Vertex& other) const
    {
        return position == other.position && uv == other.uv && normal == other.normal;
    }
};

}

namespace std
{
    template <>
    struct hash<gpro::Vertex> {
        size_t operator()(gpro::Vertex const& v) const
        {
            return ((hash<glm::vec3>()(v.position) ^ (hash<glm::vec3>()(v.normal) << 1)) >> 1) ^
                   (hash<glm::vec2>()(v.uv) << 1);
        }
    };
}  // namespace std