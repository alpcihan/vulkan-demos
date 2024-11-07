#pragma once

#include "gpro/shared.hpp"
#include "gpro/scene.hpp"
#include "gpro/scene_serializer.hpp"

namespace gpro
{
class Application {
public:

public:
    Application();

    void run();
    
    static Application& get() { return *s_instance; }
    tga::Window window() { return m_window; }
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }

private:
    static Application *s_instance;

    // window
    tga::Window m_window;
    uint32_t m_width;
    uint32_t m_height; 

    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<SceneSerializer> m_serializer;

    // render (TODO: refactor)
    std::vector<tga::RenderPass> m_forwardPasses;
    std::vector<std::vector<tga::InputSet>> m_inputSetsForwardPass;
    tga::Shader m_vertexShaderForwardPass;
    tga::Shader m_fragmentShaderForwardPass;
    tga::Buffer m_camBuffer;
    tga::Buffer m_lightsBuffer;
    tga::Buffer m_timeBuffer;

    const uint32_t m_inputSetCamAndLightIndex = 0;  // (s:0, b:0,1) camera + lights                                                                    
    const uint32_t m_inputSetDiffuseMaps = 1;       // (s:1, b:0)   diffuse maps  
    const uint32_t m_inputSetModelIndex =  2;       // (s:2, b:0)   model matrices
private:
    void _updateRenderPass(); 
};
}  // namespace gpro