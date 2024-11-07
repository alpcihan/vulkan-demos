#include "gpro/scene.hpp"

#include "gpro/application.hpp"
#include "gpro/renderer.hpp"

namespace gpro {

void Scene::init() {
    m_lights = {{glm::vec3(0, 5, 0), glm::vec3(0.4, 0.4, 0.4)}};  // TODO: load from scene config file
    m_camera = std::make_shared<CameraController>(Application::get().window(), 45,
                                                  Application::get().width() / float(Application::get().height()), 0.1f,
                                                  30000.f, glm::vec3(0, 0, -7), glm::vec3{0, 0, 1}, glm::vec3{0, 1, 0});

    Renderer::get().initCameraData(m_camera);
    Renderer::get().initLights(m_lights);
}

void Scene::addSceneObject(const SceneObject& so) {  // TODO: add move
    m_sceneObjects.push_back(so);
    Renderer::get().batch(so);
}

void Scene::onUpdate() { m_camera->update(Application::get().deltaTime()); }

}  // namespace gpro