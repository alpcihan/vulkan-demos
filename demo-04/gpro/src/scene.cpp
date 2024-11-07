#include "gpro/scene.hpp"

namespace gpro
{
void Scene::onStart()
{
    m_lights = {{glm::vec3(0, 5, 0), glm::vec3(0.4, 0.4, 0.4)}};  // TODO: load from scene config file
}

void Scene::onUpdate() {}
}  // namespace gpro