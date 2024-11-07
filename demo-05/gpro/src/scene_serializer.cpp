#pragma once

#include "gpro/scene_serializer.hpp"

#include <yaml-cpp/yaml.h>

#include <filesystem>

#include "gpro/file.hpp"
#include "gpro/utils.hpp"
#include "tga/tga_utils.hpp"

namespace gpro {

SceneSerializer::SceneSerializer(std::shared_ptr<Scene> scene) : m_scene(scene) {
    m_lastUpdateTime = std::chrono::time_point<std::chrono::file_clock>::min();
}

bool SceneSerializer::deserialize() { return _deserializeModels(); }

bool SceneSerializer::_deserializeModels() {
    bool hasDeserializedModels = false;
    bool hasUpdatedModel = false;

    // check if the models folder exist
    const std::string modelsFolderPath = gpro::resourcePath("models");
    if (!std::filesystem::exists(modelsFolderPath) || !std::filesystem::is_directory(modelsFolderPath)) {
        std::cerr << std::format("Error: Provided models folder does not exist: {0}", modelsFolderPath) << std::endl;
        return false;
    }

    // check the last modification times of the model config files
    for (const auto& entry : std::filesystem::directory_iterator(modelsFolderPath)) {
        if (entry.is_regular_file()) {
            // Get the file name
            std::string configFileName = entry.path().filename().string();

            if (configFileName.length() < 5 || configFileName.substr(configFileName.length() - 5) != ".yaml") continue;

            // get model name
            std::string modelName = configFileName.substr(0, configFileName.length() - 5);  // assumes model_name.yaml

            // check the last write time of the file or a new added file
            const std::filesystem::file_time_type fileTime = std::filesystem::last_write_time(entry.path());
            auto ftime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(fileTime);
            auto it = m_modelNameToSceneObject.find(modelName);
            if (it != m_modelNameToSceneObject.end() && ftime < m_lastUpdateTime) continue;

            // load model data
            YAML::Node n_model;  // TODO: check if corresponding texture and model file exist, before loading the yaml
            if (!_loadYAML(entry.path().string(), n_model)) continue;

            if (it == m_modelNameToSceneObject.end()) {  // add new model data
                bool result = _deserializeModel(modelName, n_model);
                if (result) hasDeserializedModels = true;
            } else  // update existing model data
            {
                /*
                std::cout << "Scene updated\n";
                glm::vec3 position;
                float scale;
                uint32_t instanceCount;
                if (!_deserializeModelConfig(n_model, transform, instanceCount)) continue;

                uint32_t sceneObjectIndex = it->second;
                auto& sceneObject = m_scene->m_sceneObjects[sceneObjectIndex];

                sceneObject.transforms.reserve(instanceCount);
                for(int i = 0; i < instanceCount; i++)
                {
                    Transform t = transform;
                    sceneObject.transforms.emplace_back(transform);
                }
                sceneObject.instanceCount = instanceCount;

                hasUpdatedModel = true;
                */
            }
        }
    }

    if (hasDeserializedModels || hasUpdatedModel) m_lastUpdateTime = std::chrono::file_clock::now();

    return hasDeserializedModels;
}

bool SceneSerializer::_deserializeModel(const std::string& modelName, const YAML::Node& data) {
    // try to get paths
    std::string modelPath = gpro::resourcePath(std::format("models/{}.obj", modelName));
    std::string modelConfigPath = gpro::resourcePath(std::format("models/{}.yaml", modelName));
    std::string modelDiffusePath = gpro::resourcePath(std::format("textures/{}.png", modelName));
    if (!std::filesystem::exists(modelPath) || !std::filesystem::exists(modelConfigPath) ||
        !std::filesystem::exists(modelDiffusePath))
        return false;

    // try to load config
    YAML::Node n_modelConfig;
    if (!_loadYAML(modelConfigPath, n_modelConfig)) return false;

    // create scene object
    Mesh mesh;
    glm::vec3 position;
    float scale;
    uint32_t instanceCount;

    // load mesh
    util::loadObj(modelPath, mesh.vertices, mesh.indices);

    // load config
    if (!_deserializeModelConfig(n_modelConfig, position, scale, instanceCount)) return false;

    std::vector<Transform> transforms;
    transforms.reserve(instanceCount);
    for (int i = 0; i < instanceCount; i++) {
        transforms.emplace_back(
            position + (float)i * glm::vec3(3, 0, 0),
            glm::vec3(0),
            glm::vec3(scale));
    }

    AABB aabb = AABB::calculateBoundingBox(mesh.vertices);
    // add the scene object to the scene
    m_scene->addSceneObject({
        std::move(mesh),
        std::move(transforms), instanceCount,
        tga::loadTexture(modelDiffusePath, tga::Format::r8g8b8a8_srgb, tga::SamplerMode::linear, tgai, true),
        std::move(aabb)
    });
    m_modelNameToSceneObject.insert(
        std::make_pair<std::string, uint32_t>(std::string(modelName), m_scene->m_sceneObjects.size() - 1));
    std::cout << std::format("added new model: {0}\n", modelName);

    return true;
}

bool SceneSerializer::_deserializeModelConfig(const YAML::Node& data, glm::vec3& position, float& scale,
                                              uint32_t& instanceCount) {
    bool isDeserialized = true;

    try {
        // deserialize instance count
        auto n_instanceCount = data["instance_count"];
        instanceCount = !n_instanceCount ? 1 : n_instanceCount.as<int32_t>();

        // deserialize position
        auto n_position = data["position"];
        glm::vec3 translation = glm::vec3(0);
        if (n_position) {
            auto xN = n_position["x"];
            auto yN = n_position["y"];
            auto zN = n_position["z"];
            translation.x = !xN ? 0 : xN.as<float>();
            translation.y = !yN ? 0 : yN.as<float>();
            translation.z = !zN ? 0 : zN.as<float>();
        }
        position = translation;

        // deserialize scale
        auto n_scale = data["scale"];
        scale = !n_scale ? 1.0f : n_scale.as<float>();

    } catch (YAML::RepresentationException e) {
        isDeserialized = false;
    }

    return isDeserialized;
}

bool SceneSerializer::_loadYAML(const std::string& path, YAML::Node& data) {
    bool isLoaded = true;
    try {
        data = YAML::LoadFile(path);
    } catch (YAML::ParserException e) {
        std::cerr << std::format("Failed to load or parse a file: '{}'\n", path) << std::endl;
        isLoaded = false;
    }

    return isLoaded;
}

}  // namespace gpro