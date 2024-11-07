#include "gpro/application.hpp"

#include "gpro/scene_serializer.hpp"
#include "gpro/utils.hpp"

#define SCREEN_SCALE 0.4

namespace gpro
{
Application *Application::s_instance = nullptr;

Application::Application()
{
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
    tgai.setWindowTitle(m_window, "gpro");

    // init scene
    m_scene = std::make_shared<gpro::Scene>();
    m_serializer = std::make_shared<SceneSerializer>(m_scene);
    std::vector<BatchUpdateInfo> _; // TODO: remove
    m_serializer->deserialize(_);
    m_scene->onStart();

    // scene camera
    m_scene->m_camera = std::make_shared<CameraController>(m_window, 90, m_width / float(m_height), 0.1f, 30000.f,
                                                           glm::vec3(0, 0, -5), glm::vec3{0, 0, 1}, glm::vec3{0, 1, 0});
    m_camBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(gpro::CamData), m_scene->m_camera->Data()});

    // scene lights
    m_lightsBuffer = gpro::util::createBuffer(tga::BufferUsage::uniform, sizeof(gpro::Light) * m_scene->m_lights.size(),
                                              toui8(m_scene->m_lights.data()), tgai);

    // scene time
    float time = 0;
    m_timeBuffer = gpro::util::createBuffer(tga::BufferUsage::uniform, sizeof(float), toui8(std::addressof(time)), tgai);

    // load the shaders
    m_vertexShaderForwardPass =
        tga::loadShader(gpro::shaderPath("indirect_phong_vert.spv"), tga::ShaderType::vertex, tgai);
    m_fragmentShaderForwardPass =
        tga::loadShader(gpro::shaderPath("indirect_phong_frag.spv"), tga::ShaderType::fragment, tgai);
}

void Application::run()
{
    _updateRenderPass();

    auto& batches = m_scene->m_batches;

    float time = 0;
    double deltaTime = 1. / 60.;
    double serializeCounter = 0;
    tga::CommandBuffer cmdBuffer{};  // single CommandBuffer that will be reused every frame

    std::vector<BatchUpdateInfo> updateInfos;

    while (!tgai.windowShouldClose(m_window)) {
        auto ts = std::chrono::steady_clock::now();
        
        if (serializeCounter > 0.2) {
            serializeCounter = 0;
            bool isDeserialized = m_serializer->deserialize(updateInfos);
            if (isDeserialized) _updateRenderPass();
        }
        
        auto nextFrame = tgai.nextFrame(m_window);
        auto cmdRecorder = tga::CommandRecorder{tgai, cmdBuffer};

        // update data
        tga::StagingBuffer timeStage = tgai.createStagingBuffer({sizeof(float), toui8(std::addressof(time))});
        cmdRecorder
            .bufferUpload(m_scene->m_camera->Data(), m_camBuffer, sizeof(gpro::CamData))
            .bufferUpload(timeStage, m_timeBuffer, sizeof(float));
        for(auto& info: updateInfos)
        {
            tga::StagingBuffer modelStage = tgai.createStagingBuffer({sizeof(Transform), toui8(std::addressof(info.model))});
            tga::StagingBuffer patternStage = tgai.createStagingBuffer({sizeof(glm::vec3), toui8(std::addressof(info.pattern))});
            tga::StagingBuffer instanceStage = tgai.createStagingBuffer({sizeof(uint32_t), toui8(std::addressof(info.instanceCount))});
            auto& batch = batches[info.batchIndex];
            cmdRecorder
                .bufferUpload(modelStage, batch.modelMatrices, sizeof(Transform), 0, sizeof(Transform) * info.index)
                .bufferUpload(patternStage, batch.patterns, sizeof(glm::vec3), 0, sizeof(glm::vec3) * info.index)
                .bufferUpload(instanceStage, batch.diicmdsBuffer, sizeof(uint32_t), 0,
                              sizeof(tga::DrawIndexedIndirectCommand) * info.index + sizeof(uint32_t));
        }
        if(updateInfos.size() > 0) updateInfos.clear();

        // forward render pass
        for (int rpi = 0; rpi < m_forwardPasses.size(); rpi++) {
            cmdRecorder
                .setRenderPass(m_forwardPasses[rpi], nextFrame, {0, 0, 0, 1})
                .bindInputSet(m_inputSetsForwardPass[rpi][m_inputSetCamAndLightIndex])  // camera + lights
                .bindInputSet(m_inputSetsForwardPass[rpi][m_inputSetDiffuseMaps]);      // diffuse maps

            for (int i = 0; i < batches.size(); i++) {
                cmdRecorder
                    .bindInputSet(m_inputSetsForwardPass[rpi][i + m_inputSetModelIndex])
                    .bindVertexBuffer(batches[i].vertexBuffer)
                    .bindIndexBuffer(batches[i].indexBuffer)
                    .drawIndexedIndirect(batches[i].diicmdsBuffer, batches[i].size);
            }
        }

        cmdBuffer = cmdRecorder.endRecording();

        // Execute commands and show the result
        tgai.execute(cmdBuffer);
        tgai.present(m_window, nextFrame);

        // Update camera
        m_scene->m_camera->update(deltaTime);
        time += deltaTime;

        // update delta time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        deltaTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - ts).count();
        serializeCounter += deltaTime;
    }
}

void Application::_updateRenderPass()
{   
    m_forwardPasses.clear();
    m_inputSetsForwardPass.clear();

    auto& batches = m_scene->m_batches;

    for (auto& batch : batches) {
        // forward pass
        // TODO: compare other multiple texture bind options (also check max texture slots)
        //      1) One renderpass, different batch textures in same set same binding (current solution)
        //      2) One renderpass, different batch textures in same set, different binding
        //      3) One renderpass, different batch textures in different sets
        //      4) Multiple renderpasses (one per batch), batch textures in same set same binding
        //      5) Check for other options
        tga::InputLayout inputLayoutForwardPass = {
            {{{tga::BindingType::uniformBuffer}, {tga::BindingType::uniformBuffer}, {tga::BindingType::uniformBuffer}}},    // Set = 0: camera, lights, time
            {{tga::BindingType::sampler, (uint32_t)batch.diffuseMaps.size()}},                                              // Set = 1: diffuse maps
            {{{tga::BindingType::uniformBuffer}, {tga::BindingType::uniformBuffer}}},                                       // Set = 2: models, patterns
        };

        m_forwardPasses.emplace_back(tgai.createRenderPass(tga::RenderPassInfo{
            m_vertexShaderForwardPass,
            m_fragmentShaderForwardPass,
            m_window,
            {},
            inputLayoutForwardPass,
            {tga::ClearOperation::all},
            {tga::CompareOperation::less},
            {tga::FrontFace::counterclockwise,
             tga::CullMode::back}}.setVertexLayout(gpro::Mesh::getVertexLayout())));
        auto& forwardPass = m_forwardPasses[m_forwardPasses.size()-1];

        // input sets
        std::vector<tga::InputSet> inputSetsForwardPass = {
            tgai.createInputSet({forwardPass, {{m_camBuffer, 0}, {m_lightsBuffer, 1}, {m_timeBuffer, 2}}, m_inputSetCamAndLightIndex}),
        };

        tga::InputSetInfo diffuseInfo{forwardPass, {}, m_inputSetDiffuseMaps};
        for (int i = 0; i < batch.diffuseMaps.size(); i++) {
            diffuseInfo.bindings.emplace_back(batch.diffuseMaps[i], 0, i);
        }
        inputSetsForwardPass.emplace_back(tgai.createInputSet(diffuseInfo));

        tga::InputSetInfo modelInfo{forwardPass, {}, m_inputSetModelIndex};
        for (auto& batch : batches) {
            tga::InputSetInfo modelInfo{forwardPass, {}, m_inputSetModelIndex};
            modelInfo.bindings = {{batch.modelMatrices, 0}, {batch.patterns, 1}};

            inputSetsForwardPass.emplace_back(tgai.createInputSet(modelInfo));
        }

        m_inputSetsForwardPass.emplace_back(std::move(inputSetsForwardPass));
    }
}

}  // namespace gpro