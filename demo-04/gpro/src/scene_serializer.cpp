#pragma once

#include "gpro/scene_serializer.hpp"

#include <yaml-cpp/yaml.h>

#include <filesystem>

#include "gpro/file.hpp"
#include "gpro/utils.hpp"
#include "tga/tga_utils.hpp"

namespace gpro
{

SceneSerializer::SceneSerializer(std::shared_ptr<Scene> scene) : m_scene(scene)
{
    m_lastUpdateTime = std::chrono::time_point<std::chrono::file_clock>::min();
}

bool SceneSerializer::deserialize(std::vector<BatchUpdateInfo>& batchUpdateInfoOut) { return _deserializeModels(batchUpdateInfoOut); }

bool SceneSerializer::_deserializeModels(std::vector<BatchUpdateInfo>& batchUpdateInfoOut)
{
    bool hasDeserializedModels = false;
    bool hasUpdatedModel = false;

    // check if the models folder exist
    const std::string modelsFolderPath = gpro::resourcePath("models");
    if (!std::filesystem::exists(modelsFolderPath) || !std::filesystem::is_directory(modelsFolderPath)) {
        std::cerr << std::format("Error: Provided models folder does not exist: {0}", modelsFolderPath) << std::endl;
        return false;
    }

    // check the last write time of the models folder
    //const std::filesystem::file_time_type folderTime = std::filesystem::last_write_time(modelsFolderPath);
    //auto time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(folderTime);
    //if (time < m_lastUpdateTime) return false;

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
            auto it = m_serializedSceneObjectMap.find(modelName);
            if (it != m_serializedSceneObjectMap.end() && ftime < m_lastUpdateTime) continue;

            // load model data
            YAML::Node n_model; // TODO: check if corresponding texture and model file exist, before loading the yaml
            if (!_loadYAML(entry.path().string(), n_model)) continue;

            if(it == m_serializedSceneObjectMap.end()) // add new model data
            {
                bool result = _deserializeModel(modelName, n_model);
                if (result) hasDeserializedModels = true;
            } else                                    // update existing model data
            {
                std::cout << "Scene updated\n";
                Transform transform;
                uint32_t instanceCount;
                glm::vec3 pattern;
                if (!_deserializeModelConfig(n_model, transform, pattern, instanceCount)) continue;

                batchUpdateInfoOut.emplace_back(
                    it->second.batchIndex,
                    it->second.index,
                    std::move(transform),
                    pattern,
                    instanceCount
                );

                hasUpdatedModel = true;
            }
        }
    }

    // IMPLEMENT: upload the batch when all the models installed
    if(hasDeserializedModels) _pushCurrentBatchToScene(false);
    if(hasDeserializedModels || hasUpdatedModel) m_lastUpdateTime = std::chrono::file_clock::now();
    
    return hasDeserializedModels;
}

bool SceneSerializer::_deserializeModel(const std::string& modelName, const YAML::Node& data)
{
    std::vector<Scene::Batch>& batches = m_scene->m_batches;

    std::vector<Vertex> vertices;
    std::vector<IndexFormat> indices;
    Transform transform;
    glm::vec3 pattern;
    uint32_t instanceCount;
    uint32_t byte = 0;

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

    // try to get config data
    if (!_deserializeModelConfig(n_modelConfig, transform, pattern, instanceCount)) return false;

    // get vertices and indices
    gpro::util::loadObj(modelPath, vertices, indices);

    // get data size
    byte = vertices.size() * sizeof(Vertex) + indices.size() * sizeof(IndexFormat) + sizeof(Transform) +
           sizeof(tga::DrawIndexedIndirectCommand);

    if (byte + m_currentBatchData.byte > MAX_BATCH_SIZE) {
        // push the current batch to gpu
        _pushCurrentBatchToScene(true);
    }

    /// continue to fill current batch (on cpu)
    // vertices
    m_currentBatchData.vertices.insert(m_currentBatchData.vertices.end(), std::make_move_iterator(vertices.begin()),
                                       std::make_move_iterator(vertices.end()));

    // indices
    m_currentBatchData.indices.insert(m_currentBatchData.indices.end(), std::make_move_iterator(indices.begin()),
                                      std::make_move_iterator(indices.end()));

    // transform
    m_currentBatchData.models.emplace_back(std::move(transform));

    // pattern
    m_currentBatchData.patterns.emplace_back(pattern);

    // draw indexed indirect command
    m_currentBatchData.diicmds.emplace_back((uint32_t)indices.size(), instanceCount, m_currentBatchData.indexOffset,
                                            m_currentBatchData.vertexOffset, (uint32_t)0);

    // increment the offsets
    m_currentBatchData.vertexOffset += vertices.size();
    m_currentBatchData.indexOffset += indices.size();

    // update size and byte
    m_currentBatchData.byte += byte;
    m_currentBatchData.size++;

    // diffuse maps
    m_currentBatchData.diffuseMaps.emplace_back(tga::loadTexture(modelDiffusePath, tga::Format::r8g8b8a8_srgb,
                                                   tga::SamplerMode::linear, tgai, true));

    // IMPLEMENT: update batch map
    std::cout << std::format("added new model: {0}\n", modelName);
    m_serializedSceneObjectMap.insert(std::make_pair<std::string, SerializedSceneObjectData>(
        std::string(modelName),
        {m_currentBatchData.index, m_currentBatchData.size - 1}
    ));

    return true;
}

bool SceneSerializer::_deserializeModelConfig(const YAML::Node& data, Transform& transform, glm::vec3& pattern, uint32_t& instanceCount)
{
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

        // deserialize scale
        auto n_scale = data["scale"];
        glm::vec3 scale = !n_scale ? glm::vec3(1) : glm::vec3(n_scale.as<float>());

        // set transform
        transform = {translation, glm::vec3(0), scale};

        // deserialize pattern
        auto n_pattern = data["pattern"];
        uint32_t i_pattern = n_pattern.as<uint32_t>();
        pattern = i_pattern == 0 ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
        
    } catch (YAML::RepresentationException e) {
        isDeserialized = false;
    }

    return isDeserialized;
}

void SceneSerializer::_pushCurrentBatchToScene(bool createNextBatchData) {
    
    auto& current = m_currentBatchData;
    Scene::Batch batch{
        gpro::util::createVertexBuffer(current.vertices, tgai),                 // vertex buffers
        gpro::util::createIndexBuffer(current.indices, tgai),                   // index buffers
        gpro::util::createBuffer(tga::BufferUsage::uniform,                     // models
                                 sizeof(Transform) * current.models.size(),
                                 tga::memoryAccess(current.models), tgai),
        gpro::util::createBuffer(tga::BufferUsage::uniform,                     // patterns
                                 sizeof(glm::vec3) * current.patterns.size(),
                                 tga::memoryAccess(current.patterns), tgai),
        current.diffuseMaps,                                                    // diffuse maps TODO: do not copy
        gpro::util::createDrawIndexedIndirectBuffer(current.diicmds, tgai),     // draw indexed indirect commands
        current.size                                                            // batch size
    };

    // push the current batch to gpu and scene
    if(m_currentBatchData.isPushed) {
        auto& current = m_currentBatchData;
        m_scene->m_batches[m_scene->m_batches.size()-1] = std::move(batch);
        std::cout << "Updated a pushed batch\n";
    } else {
        auto& current = m_currentBatchData;
        m_scene->m_batches.emplace_back(std::move(batch));
        m_currentBatchData.isPushed = true;
        std::cout << "Pushed a batch\n";
    }

    if(!createNextBatchData) return;

    // create new one
    uint32_t batchIndex = m_currentBatchData.index + 1;
    m_currentBatchData = BatchData();  // TODO: check this
    m_currentBatchData.index = batchIndex;
}

bool SceneSerializer::_loadYAML(const std::string& path, YAML::Node& data)
{
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