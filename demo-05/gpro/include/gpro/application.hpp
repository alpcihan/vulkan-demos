#pragma once

#include "gpro/scene.hpp"
#include "gpro/scene_serializer.hpp"
#include "gpro/shared.hpp"

namespace gpro {
class Application {
public:
public:
    Application();

    void run();

    static Application& get() { return *s_instance; }
    tga::Window window() { return m_window; }
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    float time() const { return m_time; }
    float deltaTime() const { return m_deltaTime; }

private:
    static Application *s_instance;

    // window
    tga::Window m_window;
    uint32_t m_width;
    uint32_t m_height;

    // time
    float m_time;
    float m_deltaTime;

    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<SceneSerializer> m_serializer;
};
}  // namespace gpro