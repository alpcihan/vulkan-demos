#include "tga/tga.hpp"
#include "tga/tga_utils.hpp"
#include "tga/tga_vulkan/tga_vulkan.hpp"
#include "utils.h"

int main()
{
    // Open the interface
    tga::Interface tgai{};

    // Load shaders
    tga::Shader vertexShader = tga::loadShader("../shaders/obj_phong_vert.spv", tga::ShaderType::vertex, tgai);
    tga::Shader fragmentShader = tga::loadShader("../shaders/obj_phong_frag.spv", tga::ShaderType::fragment, tgai);

    // Create window with the size of the screen
    auto [screenResX, screenResY] = tgai.screenResolution();
    uint32_t screenW = screenResX / 2, screenH = screenResY / 2;
    tga::Window window = tgai.createWindow({screenW, screenH});
    tgai.setWindowTitle(window, "demo-02");

    // Single CommandBuffer that will be reused every frame
    tga::CommandBuffer cmdBuffer{};

    // Load meshes
    Mesh obj1(tgai, "../models/transporter.obj");
    Mesh obj2(tgai, "../models/juf.obj");

    // Transforms
    Transform obj1Transform(glm::vec3(2,0,0), glm::vec3(0,-25,0), glm::vec3(0.006));
    tga::StagingBuffer obj1TransformDataStaging = tgai.createStagingBuffer({sizeof(obj1Transform), reinterpret_cast<uint8_t *>(std::addressof(obj1Transform))});
    tga::Buffer obj1TransformData = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(obj1Transform), obj1TransformDataStaging});

    Transform obj2Transform(glm::vec3(-2.5,0,0), glm::vec3(0,150,0), glm::vec3(0.011));
    tga::StagingBuffer obj2TransformDataStaging = tgai.createStagingBuffer({sizeof(obj2Transform), reinterpret_cast<uint8_t *>(std::addressof(obj2Transform))});
    tga::Buffer obj2TransformData = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(obj2Transform), obj2TransformDataStaging});
    
    // Camera
    Camera cam;
    cam.view = glm::lookAt( glm::vec3(0,0,-15), 
                            glm::vec3(0,0,0), 
                            glm::vec3(0,1,0) 
                      );
    cam.projection = glm::perspective(glm::radians(20.f), (float)screenW / (float)screenH, 0.1f, 100.f); // Space borders

    tga::StagingBuffer camDataStaging = tgai.createStagingBuffer({sizeof(cam), reinterpret_cast<uint8_t *>(std::addressof(cam))});
    tga::Buffer camData = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(cam), camDataStaging});

    // Light
    Light light;
    light.lightPos = glm::vec3(0,-3,-2);
    light.lightColor = glm::vec3(1,1,1);

    tga::StagingBuffer lightDataStaging = tgai.createStagingBuffer({sizeof(light), reinterpret_cast<uint8_t *>(std::addressof(light))});
    tga::Buffer lightData = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(light), lightDataStaging});

    // Renderpass 
    tga::InputLayout inputLayout({
        {{{tga::BindingType::uniformBuffer}, {tga::BindingType::uniformBuffer}}},   // Set = 0: Camera Data, Light Data
        {{{tga::BindingType::uniformBuffer}, {tga::BindingType::sampler}}},         // Set = 1: Object Data, Diffuse Map
    });

    tga::RenderPass renderPass = tgai.createRenderPass(tga::RenderPassInfo{
                                        vertexShader, fragmentShader,
                                        window,
                                        {},
                                        inputLayout,
                                        tga::ClearOperation::all,
                                        {tga::CompareOperation::less},
                                        {tga::FrontFace::clockwise, tga::CullMode::back}
                                    }
                                  .setVertexLayout(Mesh::getVertexLayout())
                                );

    // create textures
    tga::Texture obj1DiffuseMap = tga::loadTexture("../textures/transporter.png", tga::Format::r8g8b8a8_srgb, tga::SamplerMode::linear, tgai, true);
    tga::Texture obj2DiffuseMap = tga::loadTexture("../textures/juf.png", tga::Format::r8g8b8a8_srgb, tga::SamplerMode::linear, tgai, true);
    
    // create input sets
    tga::InputSet camAndLightInputSet = tgai.createInputSet({renderPass, {{camData, 0}, {lightData, 1}}, 0});

    tga::InputSet obj1TransformInputSet = tgai.createInputSet({renderPass, {{obj1TransformData, 0}, {obj1DiffuseMap, 1}}, 1});
    tga::InputSet obj2TransformInputSet = tgai.createInputSet({renderPass, {{obj2TransformData, 0}, {obj2DiffuseMap, 1}}, 1});

    while (!tgai.windowShouldClose(window)) {
        auto nextFrame = tgai.nextFrame(window);
        cmdBuffer = tga::CommandRecorder{tgai, cmdBuffer}
                        .setRenderPass(renderPass, nextFrame, {0, 0, 0, 1.})
                        .bindInputSet(camAndLightInputSet)
                        
                        .bindInputSet(obj1TransformInputSet)
                        .bindVertexBuffer(obj1.getVertexBuffer())
                        .bindIndexBuffer(obj1.getIndexBuffer())
                        .drawIndexed(obj1.getIndexCount(), 0, 0)

                        .bindInputSet(obj2TransformInputSet)
                        .bindVertexBuffer(obj2.getVertexBuffer())
                        .bindIndexBuffer(obj2.getIndexBuffer())
                        .drawIndexed(obj2.getIndexCount(), 0, 0)

                        .endRecording();

        // Execute commands and show the result
        tgai.execute(cmdBuffer);
        tgai.present(window, nextFrame);
    }

    return 0;
}