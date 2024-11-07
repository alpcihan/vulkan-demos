#include "gpro/renderer.hpp"

#include "gpro/application.hpp"
#include "gpro/utils.hpp"

#define MAX_BATCH_SIZE 10000000

#define INPUTSET_INDEX_CAM_AND_LIGHT 0     // (s:0, b:0,1) camera + lights
#define INPUTSET_INDEX_DIFFUSE_MAPS 1      // (s:1, b:0)   diffuse maps
#define INPUTSET_INDEX_MODELS 2  // (s:2, b:0,1) model matrices + aabbs

namespace gpro {

Renderer *Renderer::s_instance = nullptr;

Renderer::Renderer() {
    // set instance
    if (!s_instance)
        s_instance = this;
    else {
        std::cerr << "Renderer already exist" << std::endl;
        return;
    }
}

void Renderer::init() {
    m_batchesCPU.resize(1);

    m_vertexShader = tga::loadShader(gpro::shaderPath("indirect_phong_vert.spv"), tga::ShaderType::vertex, tgai);
    m_fragmentShader = tga::loadShader(gpro::shaderPath("indirect_phong_frag.spv"), tga::ShaderType::fragment, tgai);
    m_frustumCullingComputeShader = tga::loadShader(gpro::shaderPath("frustum_culling_comp.spv"), tga::ShaderType::compute, tgai);

    m_visibleObjectCountStaging = tgai.createStagingBuffer({sizeof(uint32_t)});
    m_visibleObjectCountBuffer = tgai.createBuffer({tga::BufferUsage::storage, sizeof(uint32_t)});
}

void Renderer::initCameraData(std::shared_ptr<CameraController>& camera) {
    // camera view and projection
    m_camStage = camera->Data();
    m_camBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(CamData), m_camStage});

    // camera frustum
    m_frustumStage = camera->getFrustumData();
    m_frustumBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(Frustum), m_frustumStage});
}

void Renderer::initLights(std::vector<Light>& lights) {
    m_lightsBuffer = gpro::util::createUniformBuffer(sizeof(Light) * lights.size(), toui8(lights.data()));
}

void Renderer::initTime(float time) {
    m_timeBuffer = gpro::util::createUniformBuffer(sizeof(float), toui8(std::addressof(time)));
}

void Renderer::batch(const SceneObject& so) {
    auto& batchCPU = m_batchesCPU[m_batchesCPU.size() - 1];

    // get data size (TODO: consider to use index count instead)
    uint32_t byte = so.mesh.vertices.size() * sizeof(Vertex) + so.mesh.indices.size() * sizeof(IndexFormat);

    if (byte + batchCPU.byte > MAX_BATCH_SIZE) {
        _flushBatch(true);
    }

    /// continue to fill current batch (on cpu)
    // vertices
    batchCPU.vertices.insert(batchCPU.vertices.end(), so.mesh.vertices.begin(), so.mesh.vertices.end());

    // indices
    batchCPU.indices.insert(batchCPU.indices.end(), so.mesh.indices.begin(), so.mesh.indices.end());

    // transform
    m_models.insert(m_models.end(), so.transforms.begin(), so.transforms.end());

    // aabb
    m_aabbs.emplace_back(so.boundingBox);

    // draw indexed indirect command
    //m_diicmds.emplace_back(so.mesh.indices.size(), so.instanceCount, batchCPU.indexOffset, batchCPU.vertexOffset, batchCPU.instanceCount);
    for(int _ = 0; _ < so.instanceCount; _++) // TODO: remove this to use per mesh diicmd. Current approach makes 1 diicmd per instance 
    {
        m_diicmds.emplace_back(so.mesh.indices.size(), 1, batchCPU.indexOffset, batchCPU.vertexOffset, batchCPU.instanceCount + _);
        m_instanceIDToMeshIDMap.push_back(batchCPU.size);
    }

    // increment the offsets
    batchCPU.vertexOffset += so.mesh.vertices.size();
    batchCPU.indexOffset += so.mesh.indices.size();

    // update size and byte
    batchCPU.byte += byte;
    batchCPU.size++;
    batchCPU.instanceCount += so.instanceCount;

    // diffuse maps
    batchCPU.diffuseMaps.push_back(so.diffuseMap);

    _flushBatch(false);
}

void Renderer::_flushBatch(bool createNextBatchData) {
    auto& batchCPU = m_batchesCPU[m_batchesCPU.size() - 1];
   
    BatchGPU batch{
        gpro::util::createVertexBuffer(batchCPU.vertices),              // vertex buffers
        gpro::util::createIndexBuffer(batchCPU.indices),                // index buffers
        batchCPU.diffuseMaps,                                           // diffuse maps TODO: do not copy
        batchCPU.size,                                                  // batch size
        batchCPU.instanceCount,                                         // total instance count in a batch,
        0
    };

    tgai.free(m_modelsBuffer);
    tgai.free(m_aabbsBuffer);
    tgai.free(m_diicmdsBuffer);
    tgai.free(m_instanceIDToMeshIDMapBuffer);

    m_modelsBuffer = gpro::util::createStorageBuffer(sizeof(Transform) * m_models.size(), tga::memoryAccess(m_models));
    m_aabbsBuffer = gpro::util::createStorageBuffer(sizeof(AABB) * m_aabbs.size(), tga::memoryAccess(m_aabbs));
    m_diicmdsBuffer = gpro::util::createDrawIndexedIndirectBuffer(m_diicmds);
    m_instanceIDToMeshIDMapBuffer = gpro::util::createStorageBuffer(sizeof(uint32_t) * m_instanceIDToMeshIDMap.size(), tga::memoryAccess(m_instanceIDToMeshIDMap));

    bool isNewBatch = false;  // true -> new, false -> updated
    // push the current batch to gpu
    if (batchCPU.isPushed) {
        auto& current = m_batchesGPU[m_batchesGPU.size() - 1];
        tgai.free(current.verticesBuffer);
        tgai.free(current.indicesBuffer);
        current = std::move(batch);
        std::cout << "Updated a pushed batch\n";
    } else {
        m_batchesGPU.emplace_back(std::move(batch));
        batchCPU.isPushed = true;
        isNewBatch = true;
        std::cout << "Pushed a batch\n";
    }

    // update the last render pass (or push new one)
    _updateRenderPass(isNewBatch);
    _updateFrustumCullingPass();

    if (!createNextBatchData) return;

    // create new one
    uint32_t batchIndex = batchCPU.index + 1;
    m_batchesCPU.emplace_back();
    m_batchesCPU[m_batchesCPU.size() - 1].index = batchIndex;
}

void Renderer::render(tga::Window& window) {
    auto nextFrame = tgai.nextFrame(window);
    auto cmdRecorder = tga::CommandRecorder{tgai, m_cmdBuffer};

    // frustum culling pass
    uint32_t* visibleObjectCount = static_cast<uint32_t *>(tgai.getMapping(m_visibleObjectCountStaging));
    *visibleObjectCount = 0;
    const uint32_t instanceCount = m_models.size();
    constexpr auto workGroupSize = 64;
    auto cmd = tga::CommandRecorder(tgai)
        .setComputePass(m_frustumCullingPass)
        .bufferUpload(m_visibleObjectCountStaging, m_visibleObjectCountBuffer, sizeof(uint32_t))
        .bindInputSet(m_frustumCullingPassInputSet)
        .dispatch((instanceCount + (workGroupSize - 1)) / workGroupSize, 1, 1)
        .endRecording();
    tgai.execute(cmd);
    tgai.waitForCompletion(cmd);

    auto getResultCmd = tga::CommandRecorder(tgai)
        .barrier(tga::PipelineStage::ComputeShader, tga::PipelineStage::Transfer)
        .bufferDownload(m_visibleObjectCountBuffer, m_visibleObjectCountStaging, sizeof(uint32_t))
        .endRecording();
    tgai.execute(getResultCmd);
    tgai.waitForCompletion(getResultCmd);

    std::cout << std::format("Visible object count: {0}\n",*visibleObjectCount);

    // update data (TODO: update before culling, currently runs 1 frame behind)
    cmdRecorder.bufferUpload(m_camStage, m_camBuffer, sizeof(gpro::CamData))
        .bufferUpload(m_frustumStage, m_frustumBuffer, sizeof(Frustum));
    
    // forward render pass
    for (int i = 0; i < m_renderPasses.size(); i++) {
        cmdRecorder
            .setRenderPass(m_renderPasses[i], nextFrame, {0, 0, 0, 1})
            .bindVertexBuffer(m_batchesGPU[i].verticesBuffer)
            .bindIndexBuffer(m_batchesGPU[i].indicesBuffer)
            .bindInputSet(m_inputSets[i][INPUTSET_INDEX_CAM_AND_LIGHT])             // camera + lights
            .bindInputSet(m_inputSets[i][INPUTSET_INDEX_DIFFUSE_MAPS])              // diffuse maps
            .bindInputSet(m_inputSets[i][INPUTSET_INDEX_MODELS])                    // model + aabbs
            .drawIndexedIndirect(m_diicmdsBuffer, m_batchesGPU[i].instanceCount);
    }

    m_cmdBuffer = cmdRecorder.endRecording();
    tgai.execute(m_cmdBuffer);
    tgai.present(window, nextFrame);
}

void Renderer::_updateRenderPass(bool isNewRenderPass) {
    uint32_t index = m_batchesGPU.size() - 1;
    auto& batch = m_batchesGPU[index];

    // input layout
    const tga::InputLayout inputLayoutForwardPass{
        {
            // S0
            {tga::BindingType::uniformBuffer},  // B0: VP
            {tga::BindingType::uniformBuffer},  // B1: frustum
            {tga::BindingType::uniformBuffer},  // B2: lights
            {tga::BindingType::uniformBuffer},  // B3: time
        },
        {
            // S1
            {tga::BindingType::sampler, (uint32_t)batch.diffuseMaps.size()}  // B0: diffuse maps
        },
        {
            // S2
            {tga::BindingType::storageBuffer},  // B0: models
            {tga::BindingType::storageBuffer},  // B1: instance id to mesh id map
        },
    };

    // render pass
    tga::RenderPass renderPass = tgai.createRenderPass(tga::RenderPassInfo{
        m_vertexShader,
        m_fragmentShader,
        Application::get().window(),
        {},
        inputLayoutForwardPass,
        {tga::ClearOperation::all},
        {tga::CompareOperation::less},
        {tga::FrontFace::counterclockwise,
         tga::CullMode::back}}.setVertexLayout(gpro::Mesh::getVertexLayout()));

    // input sets - camera, light, time
    std::vector<tga::InputSet> inputSetsRenderPass{
        tgai.createInputSet({renderPass,
                             {{m_camBuffer, 0}, {m_frustumBuffer, 1}, {m_lightsBuffer, 2}, {m_timeBuffer, 3}},
                             INPUTSET_INDEX_CAM_AND_LIGHT}),
    };

    // input sets - diffuse maps
    {
        tga::InputSetInfo info{renderPass, {}, INPUTSET_INDEX_DIFFUSE_MAPS};
        for (int i = 0; i < batch.diffuseMaps.size(); i++) {
            info.bindings.emplace_back(batch.diffuseMaps[i], 0, i);
        }
        inputSetsRenderPass.emplace_back(tgai.createInputSet(info));
    }

    // input sets - model matrices, instance id to mesh id map
    {
        tga::InputSetInfo info{renderPass, {}, INPUTSET_INDEX_MODELS};
        info.bindings = {{m_modelsBuffer, 0}, {m_instanceIDToMeshIDMapBuffer,1}};
        inputSetsRenderPass.emplace_back(tgai.createInputSet(info));
    }

    if (isNewRenderPass) {
        m_renderPasses.emplace_back(std::move(renderPass));
        m_inputSets.emplace_back(std::move(inputSetsRenderPass));
    } else {
        m_inputSets[index].clear();
        m_renderPasses[index] = std::move(renderPass);
        m_inputSets[index] = std::move(inputSetsRenderPass);
    }
}

void Renderer::_updateFrustumCullingPass() {
    const tga::InputLayout inputLayout{{
        // S0
        {tga::BindingType::storageBuffer},  // B0 models
        {tga::BindingType::storageBuffer},  // B1 aabbs
        {tga::BindingType::uniformBuffer},  // B2 camera VP
        {tga::BindingType::storageBuffer},  // B3 instance id to mesh id map
        {tga::BindingType::storageBuffer},  // B4 visible object count (writeonly)
        {tga::BindingType::storageBuffer},  // B5 diicmds (writeonly)
    }};

    m_frustumCullingPass = tgai.createComputePass({m_frustumCullingComputeShader, inputLayout});

    m_frustumCullingPassInputSet = tgai.createInputSet(
        {m_frustumCullingPass,
         {{m_modelsBuffer, 0}, {m_aabbsBuffer, 1}, {m_camBuffer, 2}, {m_instanceIDToMeshIDMapBuffer, 3}, {m_visibleObjectCountBuffer, 4}, {m_diicmdsBuffer, 5}},
         0});
}
}  // namespace gpro