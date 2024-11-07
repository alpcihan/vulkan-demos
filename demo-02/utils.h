#pragma once
#include "tga/tga.hpp"
#include <glm/gtc/matrix_transform.hpp>

typedef uint32_t IndexFormat;

struct Transform {
    Transform(glm::vec3 translation, glm::vec3 rotation = glm::vec3(0.0), glm::vec3 scale = glm::vec3(1.0))
    {
        transform = glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(glm::quat(glm::radians(rotation))) * glm::scale(glm::mat4(1.0f), scale);
    }

    alignas(16) glm::mat4 transform = glm::mat4(1);
};

struct Camera {
    alignas(16) glm::mat4 view = glm::mat4(1);
    alignas(16) glm::mat4 projection = glm::mat4(1);
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

namespace std
{
template <>
struct hash<Vertex> {
    size_t operator()(Vertex const& v) const
    {
        return ((hash<glm::vec3>()(v.position) ^ (hash<glm::vec3>()(v.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(v.uv) << 1);
    }
};
}  // namespace std

namespace util
{

void loadObj(const std::string& objFilePath, std::vector<Vertex>& vBuffer, std::vector<IndexFormat>& iBuffer)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objFilePath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::vector<Vertex> preVertexBuffer;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.position = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                               attrib.vertices[3 * index.vertex_index + 2]};
            vertex.normal = {attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1],
                              attrib.normals[3 * index.normal_index + 2]};
            vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                         1.f - attrib.texcoords[2 * index.texcoord_index + 1]};
            preVertexBuffer.emplace_back(vertex);
        }
    }

    std::unordered_map<Vertex, uint32_t> foundVertices{};
    vBuffer.clear();
    iBuffer.clear();

    for (const auto& vertex : preVertexBuffer) {
        if (!foundVertices.count(vertex)) {  // It's a new Vertex
            foundVertices[vertex] = static_cast<uint32_t>(vBuffer.size());
            vBuffer.emplace_back(vertex);
        } else {  // Seen before, average the the tangents
            auto& v = vBuffer[foundVertices[vertex]];
            // v.tangent += vertex.tangent;
            // v.bitangent += vertex.bitangent;
        }
        iBuffer.emplace_back(foundVertices[vertex]);
    }
    preVertexBuffer.clear();
}

}

class Mesh
{
public:
    Mesh(tga::Interface& tgai, const std::string& objFilePath)
    {
        util::loadObj(objFilePath, m_vertices, m_indices);

        // create vertex buffer
        tga::StagingBuffer vertexStaging = tgai.createStagingBuffer({m_vertices.size() * sizeof(Vertex), tga::memoryAccess(m_vertices)});
        m_vertexBuffer = tgai.createBuffer({tga::BufferUsage::vertex, m_vertices.size() * sizeof(Vertex), vertexStaging});

        // create index buffer
        tga::StagingBuffer indexStaging = tgai.createStagingBuffer({m_indices.size() * sizeof(IndexFormat), tga::memoryAccess(m_indices)});
        m_indexBuffer = tgai.createBuffer({tga::BufferUsage::index, m_indices.size() * sizeof(IndexFormat), indexStaging});
    }

    inline const tga::Buffer& getVertexBuffer() const { return m_vertexBuffer; } 
    inline const tga::Buffer& getIndexBuffer() const { return m_indexBuffer; }
    inline uint32_t getIndexCount() const { return m_indices.size(); }

    static inline const tga::VertexLayout& getVertexLayout() {
        static tga::VertexLayout vertexLayout(sizeof(Vertex), {{offsetof(Vertex, position), tga::Format::r32g32b32_sfloat},
                                                    {offsetof(Vertex, uv), tga::Format::r32g32_sfloat},
                                                    {offsetof(Vertex, normal), tga::Format::r32g32b32_sfloat}});
        return vertexLayout;
    }

private:
    tga::Buffer m_vertexBuffer;
    tga::Buffer m_indexBuffer;

    std::vector<Vertex> m_vertices;
    std::vector<IndexFormat> m_indices;
};