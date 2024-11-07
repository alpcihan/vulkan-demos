#include "gpro/mesh.hpp"

namespace gpro
{

Mesh::Mesh(tga::Interface& tgai, const std::string& objFilePath)
{
    // load obj
    std::vector<Vertex> m_vertices;
    std::vector<IndexFormat> m_indices;
    util::loadObj(objFilePath, m_vertices, m_indices);
    m_indexCount = m_indices.size();

    // create vertex buffer
    tga::StagingBuffer vertexStaging =
        tgai.createStagingBuffer({m_vertices.size() * sizeof(Vertex), tga::memoryAccess(m_vertices)});
    m_vertexBuffer = tgai.createBuffer({tga::BufferUsage::vertex, m_vertices.size() * sizeof(Vertex), vertexStaging});

    // create index buffer
    tga::StagingBuffer indexStaging =
        tgai.createStagingBuffer({m_indices.size() * sizeof(IndexFormat), tga::memoryAccess(m_indices)});
    m_indexBuffer = tgai.createBuffer({tga::BufferUsage::index, m_indices.size() * sizeof(IndexFormat), indexStaging});
}

}  // namespace gpro