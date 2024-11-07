#pragma once

#include "gpro/camera_controller.hpp"
#include "gpro/components.hpp"
#include "gpro/shared.hpp"
#include "gpro/utils.hpp"

namespace gpro {

class Scene {
public:
    void init();
    void addSceneObject(const SceneObject& so);
    void onUpdate();

private:
    std::vector<SceneObject> m_sceneObjects;
    std::shared_ptr<gpro::CameraController> m_camera;
    std::vector<gpro::Light> m_lights;

    friend class Application;
    friend class SceneSerializer;
};

}  // namespace gpro