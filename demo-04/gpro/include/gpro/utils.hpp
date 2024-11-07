#pragma once

#include "gpro/shared.hpp"

namespace gpro::util
{

tga::Buffer createBuffer(tga::BufferUsage usage, size_t size, uint8_t const *_data, tga::Interface& tgai);
tga::Buffer createVertexBuffer(std::vector<Vertex>& vertices, tga::Interface& tgai);
tga::Buffer createIndexBuffer(std::vector<IndexFormat>& indices, tga::Interface& tgai);
tga::Buffer createDrawIndexedIndirectBuffer(std::vector<tga::DrawIndexedIndirectCommand> diicmds, tga::Interface& tgai);

uint64_t UUID();
glm::vec3 rnd3();

void loadObj(const std::string& objFilePath, std::vector<Vertex>& vBuffer, std::vector<IndexFormat>& iBuffer);

}  // namespace gpro