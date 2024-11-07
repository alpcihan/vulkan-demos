#pragma once

#include "gpro/camera_controller.hpp"
#include "gpro/mesh.hpp"
#include "gpro/shared.hpp"

namespace gpro
{

struct SceneObject  // TODO: use ecs
{
    Mesh mesh;
    Transform transform;
    uint32_t instanceCount;
};

class Scene {
public:
    std::vector<SceneObject> sceneObjects;

public:
    void onStart();
    void onUpdate();

private:
    std::shared_ptr<gpro::CameraController> m_camera;

    std::vector<gpro::Light> m_lights;

    struct Batch  // TODO: create different systems for dynamic and static batches
    {
        tga::Buffer vertexBuffer;
        tga::Buffer indexBuffer;
        tga::Buffer modelMatrices;
        tga::Buffer patterns;
        std::vector<tga::Texture> diffuseMaps;
        tga::Buffer diicmdsBuffer;
        uint32_t size = 0;
    };
    std::vector<Batch> m_batches;

    friend class Application;
    friend class SceneSerializer;
};

}  // namespace gpro