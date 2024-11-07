#pragma once

#include <yaml-cpp/yaml.h>

#include "gpro/scene.hpp"
#include "gpro/shared.hpp"

namespace gpro {

class SceneSerializer {
public:
    SceneSerializer(std::shared_ptr<Scene> scene);
    bool deserialize();

private:
    std::shared_ptr<Scene> m_scene;

    std::unordered_map<std::string, uint32_t> m_modelNameToSceneObject;

    // Initialize start time
    std::chrono::time_point<std::chrono::file_clock> m_lastUpdateTime;

private:
    bool _deserializeModels();
    bool _deserializeModel(const std::string& modelName, const YAML::Node& data);
    bool _deserializeModelConfig(const YAML::Node& data, glm::vec3& position, float& scale, uint32_t& instanceCount);
    bool _loadYAML(const std::string& path, YAML::Node& data);
};

}  // namespace gpro