#pragma once

#include "gpro/gpro.hpp"

namespace gpro::util
{

void loadObj(const std::string& objFilePath, std::vector<Vertex>& vBuffer, std::vector<IndexFormat>& iBuffer);

tga::Buffer createBuffer(tga::BufferUsage usage, size_t size, uint8_t const *_data, tga::Interface& tgai);

}  // namespace gpro