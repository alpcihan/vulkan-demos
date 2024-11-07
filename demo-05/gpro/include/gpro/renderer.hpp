#pragma once

#include "gpro/camera_controller.hpp"
#include "gpro/shared.hpp"
#include "gpro/components.hpp"

namespace gpro {

class Renderer {
public:
    Renderer();
    static Renderer& get() { return *s_instance; }

    void init();

    void initCameraData(std::shared_ptr<CameraController>& camera);
    void initLights(std::vector<Light>& lights);
    void initTime(float time);

    void batch(const SceneObject& so);

    void render(tga::Window& window);

private:
    struct BatchGPU  { // TODO: create different systems for dynamic and static batches
        tga::Buffer verticesBuffer;
        tga::Buffer indicesBuffer;
        std::vector<tga::Texture> diffuseMaps;
        uint32_t size = 0;
        uint32_t instanceCount = 0;
        uint32_t diicmdOffset = 0;
    };
    std::vector<BatchGPU> m_batchesGPU;
    tga::Buffer m_modelsBuffer;                 // per instance
    tga::Buffer m_aabbsBuffer;                  // per mesh
    tga::Buffer m_visibleObjectCountBuffer;     // per instance
    tga::Buffer m_diicmdsBuffer;                // per instance (TODO: make it work per mesh)
    tga::Buffer m_instanceIDToMeshIDMapBuffer;  // per instance

    struct BatchCPU { // TODO: use staging buffer with mapping instead of duplicate data
        BatchCPU() : vertexOffset(0), indexOffset(0), size(0), byte(0), index(0), isPushed(false) {}

        int32_t vertexOffset = 0;
        uint32_t indexOffset = 0;
        uint32_t size = 0;                                      // mesh count
        uint32_t byte = 0;                                      // byte
        uint32_t index = 0;                                     // batch index  
        uint32_t instanceCount = 0;                             // total number of instance
        std::vector<Vertex> vertices;
        std::vector<IndexFormat> indices;
        std::vector<tga::Texture> diffuseMaps;
        bool isPushed = false;
    };
    std::vector<BatchCPU> m_batchesCPU;
    std::vector<Transform> m_models;
    std::vector<AABB> m_aabbs;
    tga::StagingBuffer m_visibleObjectCountStaging;
    std::vector<tga::DrawIndexedIndirectCommand> m_diicmds;
    std::vector<uint32_t> m_instanceIDToMeshIDMap;

    struct BatchedObjectData {
        uint32_t batchIndex;
        uint32_t index;
    };
    std::unordered_map<uint32_t, BatchedObjectData> m_batchedObjectMap;

    tga::CommandBuffer m_cmdBuffer{};

    // render passes
    std::vector<tga::RenderPass> m_renderPasses;
    std::vector<std::vector<tga::InputSet>> m_inputSets;
    tga::Shader m_vertexShader;
    tga::Shader m_fragmentShader;

    // frustum culling pass
    tga::ComputePass m_frustumCullingPass;
    tga::InputSet m_frustumCullingPassInputSet;
    tga::Shader m_frustumCullingComputeShader;

    // uniforms
    tga::Buffer m_camBuffer, m_frustumBuffer;       // camera buffers
    tga::StagingBuffer m_camStage, m_frustumStage;  // camera stages
    tga::Buffer m_lightsBuffer;                     // lights buffer
    tga::Buffer m_timeBuffer;                       // app time buffer

private:
    void _flushBatch(bool createNextBatchData);
    void _updateRenderPass(bool isNewRenderPass);
    void _updateFrustumCullingPass();

private:
    static Renderer *s_instance;
};
}  // namespace gpro