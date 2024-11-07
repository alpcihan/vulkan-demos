#pragma once

#include <chrono>
#include <ctime>
#include <glm/gtc/random.hpp>
#include <limits>
#include <thread>

#include "gpro/file.hpp"
#include "gpro/tga_interface.hpp"
#include "tga/tga.hpp"
#include "tga/tga_math.hpp"
#include "tga/tga_utils.hpp"
#include "tga/tga_vulkan/tga_vulkan.hpp"

#define toui8 reinterpret_cast<uint8_t *>

namespace gpro
{

typedef uint32_t IndexFormat;

struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;

    bool operator==(const Vertex& other) const
    {
        return position == other.position && uv == other.uv && normal == other.normal;
    }
};

}  // namespace gpro

namespace std
{
template <>
struct hash<gpro::Vertex> {
    size_t operator()(gpro::Vertex const& v) const
    {
        return ((hash<glm::vec3>()(v.position) ^ (hash<glm::vec3>()(v.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(v.uv) << 1);
    }
};
}  // namespace std
