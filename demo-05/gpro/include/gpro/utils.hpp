#pragma once

#include "gpro/shared.hpp"

namespace gpro::util {

tga::Buffer createBuffer(tga::BufferUsage usage, size_t size, uint8_t const *data);
tga::Buffer createUniformBuffer(size_t size, uint8_t const *data);
tga::Buffer createStorageBuffer(size_t size, uint8_t const *data);
tga::Buffer createVertexBuffer(std::vector<Vertex>& vertices);
tga::Buffer createIndexBuffer(std::vector<IndexFormat>& indices);
tga::Buffer createDrawIndexedIndirectBuffer(std::vector<tga::DrawIndexedIndirectCommand> diicmds);

glm::vec3 rnd3();

void loadObj(const std::string& objFilePath, std::vector<Vertex>& vBuffer, std::vector<IndexFormat>& iBuffer);

// A little helper function to create a staging buffer that acts like a specific type
template <typename T>
std::tuple<T&, tga::StagingBuffer, size_t> stagingBufferOfType(tga::Interface& tgai);
}  // namespace gpro::util