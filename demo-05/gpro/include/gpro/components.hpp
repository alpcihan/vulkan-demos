#pragma once

#include "gpro/shared.hpp"

namespace gpro {

struct Light {
    alignas(16) glm::vec3 lightPos = glm::vec3(0);  // TODO: use transform component instead
    alignas(16) glm::vec3 lightColor = glm::vec3(1);
};

struct Transform {
    alignas(16) glm::mat4 transform = glm::mat4(1);

    Transform(glm::vec3 translation = glm::vec3(0), glm::vec3 rotation = glm::vec3(0), glm::vec3 scale = glm::vec3(1)) {
        transform = glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(glm::quat(glm::radians(rotation))) *
                    glm::scale(glm::mat4(1.0f), scale);
    }
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<IndexFormat> indices;

    static const tga::VertexLayout& getVertexLayout() {
        static tga::VertexLayout vertexLayout(sizeof(Vertex),
                                              {{offsetof(Vertex, position), tga::Format::r32g32b32_sfloat},
                                               {offsetof(Vertex, uv), tga::Format::r32g32_sfloat},
                                               {offsetof(Vertex, normal), tga::Format::r32g32b32_sfloat}});
        return vertexLayout;
    }
};

struct AABB {
    alignas(16) glm::vec3 mn;
    alignas(16) glm::vec3 mx;

    AABB(const glm::vec3& min, const glm::vec3& max)
        : mn(min), mx(max) {}

    static AABB calculateBoundingBox(const std::vector<glm::vec3>& points);
    static AABB calculateBoundingBox(const std::vector<Vertex>& vertices);
};

struct SceneObject {  // TODO: use ecs
                      // TODO: this is not a component
    Mesh mesh;
    std::vector<Transform> transforms;
    uint32_t instanceCount;
    tga::Texture diffuseMap;
    AABB boundingBox;
};

};  // namespace gpro