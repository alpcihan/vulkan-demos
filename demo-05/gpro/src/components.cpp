#include "gpro/components.hpp"

namespace gpro {

AABB AABB::calculateBoundingBox(const std::vector<glm::vec3>& points) {
    glm::vec3 mx(std::numeric_limits<float>::min());
    glm::vec3 mn(std::numeric_limits<float>::max());

    for (const auto& point : points) {
        mx = glm::max(point, mx);
        mn = glm::min(point, mn);
    }

    return {mx, mn};
}

AABB AABB::calculateBoundingBox(const std::vector<Vertex>& vertices) {
    glm::vec3 mx(std::numeric_limits<float>::min());
    glm::vec3 mn(std::numeric_limits<float>::max());

    for (const auto& vertex : vertices) {
        mx = glm::max(vertex.position, mx);
        mn = glm::min(vertex.position, mn);
    }

    return {mn, mx};
}

}  // namespace gpro