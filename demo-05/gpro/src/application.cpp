#include "gpro/application.hpp"

#include "gpro/renderer.hpp"
#include "gpro/scene_serializer.hpp"
#include "gpro/utils.hpp"

#define SCREEN_SCALE 0.4

namespace gpro {
    
Application *Application::s_instance = nullptr;

Renderer renderer;

Application::Application() {
    // set instance
    if (!s_instance)
        s_instance = this;
    else {
        std::cerr << "Application already exist" << std::endl;
        return;
    }

    // init window
    auto [screenX, screenY] = tgai.screenResolution();
    m_width = screenX * SCREEN_SCALE;
    m_height = screenY * SCREEN_SCALE;
    m_window = tgai.createWindow({m_width, m_height});
    tgai.setWindowTitle(m_window, "demo-05");

    // init time
    m_time = 0;
    m_deltaTime = 1. / 60.;

    // init renderer
    Renderer::get().init();
    Renderer::get().initTime(m_time);

    // init scene
    m_scene = std::make_shared<gpro::Scene>();
    m_scene->init();
    m_serializer = std::make_shared<SceneSerializer>(m_scene);
    m_serializer->deserialize();
}

void Application::run() {
    double sceneSerializeTimer = 0;
    double messageTimer = 0;
    while (!tgai.windowShouldClose(m_window)) {
        // init time
        auto ts = std::chrono::steady_clock::now();
    
        if (sceneSerializeTimer > 0.2) {
            sceneSerializeTimer = 0;
            m_serializer->deserialize();
        }

        m_scene->onUpdate();
        Renderer::get().render(m_window);

        // update time
        m_time += m_deltaTime;
        m_deltaTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - ts).count();
        sceneSerializeTimer += m_deltaTime;
        messageTimer += m_deltaTime;

        //if (messageTimer > 0.2) {
        //    messageTimer = 0;
        //    std::cout << std::format("fps: {:.0f}\n\n", 1 / m_deltaTime);
        //}
    }
}

}  // namespace gpro