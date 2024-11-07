#pragma once

#include "gpro/gpro.hpp"

namespace gpro
{

class Mesh {
public:
    Mesh(tga::Interface& tgai, const std::string& objFilePath);

    const tga::Buffer& getVertexBuffer() const { return m_vertexBuffer; }
    const tga::Buffer& getIndexBuffer() const { return m_indexBuffer; }
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
    tga::Buffer m_vertexBuffer;
    tga::Buffer m_indexBuffer;

    uint32_t m_indexCount;
};

}  // namespace gpro