#pragma once

#include <yaml-cpp/yaml.h>

#include "gpro/scene.hpp"
#include "gpro/shared.hpp"

namespace gpro
{

struct BatchUpdateInfo
{
    uint32_t batchIndex;
    uint32_t index;
    Transform model;
    glm::vec3 pattern;
    uint32_t instanceCount;
};

class SceneSerializer {
public:
    SceneSerializer(std::shared_ptr<Scene> scene);

    bool deserialize(std::vector<BatchUpdateInfo>& batchUpdateInfoOut);

private:
    std::shared_ptr<Scene> m_scene;

    struct SerializedSceneObjectData {
        uint32_t batchIndex;
        uint32_t index;
    };
    std::unordered_map<std::string, SerializedSceneObjectData> m_serializedSceneObjectMap;

    struct BatchData {
        BatchData() : vertexOffset(0), indexOffset(0), size(0),
                      byte(0), index(0), isPushed(false) {}

        int32_t vertexOffset = 0;
        uint32_t indexOffset = 0;
        uint32_t size = 0;
        uint32_t byte = 0;
        uint32_t index = 0;
        std::vector<Vertex> vertices;
        std::vector<IndexFormat> indices;
        std::vector<Transform> models;
        std::vector<glm::vec3> patterns;
        std::vector<tga::Texture> diffuseMaps;
        std::vector<tga::DrawIndexedIndirectCommand> diicmds;
        bool isPushed = false;
    };
    BatchData m_currentBatchData;

    // Initialize start time
    std::chrono::time_point<std::chrono::file_clock> m_lastUpdateTime;

private:
    bool _deserializeModels(std::vector<BatchUpdateInfo>& batchUpdateInfoOut);
    bool _deserializeModel(const std::string& modelName, const YAML::Node& data);
    bool _deserializeModelConfig(const YAML::Node& data, Transform& transform, glm::vec3& pattern, uint32_t& instanceCount);

    void _pushCurrentBatchToScene(bool createNextBatchData);

    bool _loadYAML(const std::string& path, YAML::Node& data);
};

}  // namespace gpro