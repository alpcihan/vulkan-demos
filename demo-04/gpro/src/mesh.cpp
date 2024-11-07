#include "gpro/mesh.hpp"
#include "gpro/utils.hpp"

namespace gpro
{

Mesh::Mesh(const std::string& objFilePath)
{
    util::loadObj(objFilePath, m_vertices, m_indices);
    m_indexCount = m_indices.size();
}

}  // namespace gpro