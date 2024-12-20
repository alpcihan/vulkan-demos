#include "gpro/utils.hpp"

namespace gpro::util
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

tga::Buffer createBuffer(tga::BufferUsage usage, size_t size, uint8_t const *data, tga::Interface& tgai)
{
    tga::StagingBuffer stagingBuffer = tgai.createStagingBuffer({size, data});
    return tgai.createBuffer({usage, size, stagingBuffer});
}

}  // namespace gpro::util