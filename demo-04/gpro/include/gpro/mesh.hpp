#pragma once

#include "gpro/data.hpp"
#include "gpro/shared.hpp"

namespace gpro
{

class Mesh {
public:
    Mesh(const std::string& objFilePath);

    std::vector<Vertex>& getVertices() { return m_vertices; }     // TODO: make const return type
    std::vector<IndexFormat>& getIndices() { return m_indices; }  // TODO: make const return type
    uint32_t getIndexCount() const { return m_indexCount; }

    static const tga::VertexLayout& getVertexLayout()
    {
        static tga::VertexLayout vertexLayout(sizeof(Vertex),
                                              {{offsetof(Vertex, position), tga::Format::r32g32b32_sfloat},
                                               {offsetof(Vertex, uv), tga::Format::r32g32_sfloat},
                                               {offsetof(Vertex, normal), tga::Format::r32g32b32_sfloat}});
        return vertexLayout;
    }

private:
    std::vector<Vertex> m_vertices;
    std::vector<IndexFormat> m_indices;

    uint32_t m_indexCount;
};

}  // namespace gpro