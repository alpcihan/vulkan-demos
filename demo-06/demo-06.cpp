#include <chrono>
#include <ctime>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <thread>

#include "file.hpp"
#include "tga/tga.hpp"
#include "tga/tga_utils.hpp"
#include "tga/tga_vulkan/tga_vulkan.hpp"

/* Data types */
struct Model {
    // triangle data
    uint32_t offset = 0;
    uint32_t stride = 0;
    // material
    uint32_t diffuseMapId = 0;
    alignas(16) glm::vec3 diffuseColor = glm::vec3(1);
};

struct SphereLight {
    alignas(16) glm::vec3 position = glm::vec3(0);
    alignas(16) glm::vec3 emissiveColor = glm::vec3(1);
    float radius = 1.0f;
};

struct Triangle {
    alignas(16) glm::vec3 v0;
    alignas(16) glm::vec3 v1;
    alignas(16) glm::vec3 v2;
    alignas(8) glm::vec2 uv0;
    alignas(8) glm::vec2 uv1;
    alignas(8) glm::vec2 uv2;
    alignas(16) glm::vec3 normal;
};

struct Transform {
    alignas(16) glm::mat4 transform = glm::mat4(1);

    Transform(glm::vec3 translation = glm::vec3(0), glm::vec3 rotation = glm::vec3(0), glm::vec3 scale = glm::vec3(1)) {
        transform = glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(glm::quat(glm::radians(rotation))) *
                    glm::scale(glm::mat4(1.0f), scale);
    }
};

struct ModelCPU {
    std::string name;
    Transform transform;
    bool invertNormals = false;
    std::string diffuseMapName = "";
    glm::vec3 diffuseColor = glm::vec3(1.0f);
};

struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;

    bool operator==(const Vertex& other) const {
        return position == other.position && uv == other.uv && normal == other.normal;
    }
};

struct Camera {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
};

namespace std {
template <>
struct hash<Vertex> {
    size_t operator()(Vertex const& v) const {
        return ((hash<glm::vec3>()(v.position) ^ (hash<glm::vec3>()(v.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(v.uv) << 1);
    }
};
}  // namespace std

// camera controller
class CameraController {
public:
    CameraController(tga::Window window, float fov, float aspectRatio, float nearPlane, float farPlane,
                     glm::vec3 position, glm::vec3 front, glm::vec3 up, tga::Interface& tgai)
        : m_window(window), m_camStaging(tgai.createStagingBuffer({sizeof(*m_camData)})),
          m_camData(static_cast<Camera *>(tgai.getMapping(m_camStaging))), m_fov(fov), m_aspectRatio(aspectRatio), m_nearPlane(nearPlane),
          m_farPlane(farPlane), m_position(position), m_front(front), m_up(up), m_right(glm::cross(up, front)) {
        updateData();
    }
    void update(float deltaTime, tga::Interface& tgai) {
        m_isUpdated = false;
        processInput(deltaTime, tgai);
        updateData();
    }

    glm::vec3 position() { return m_position; }
    glm::mat4& view() { return m_camData->view; }
    glm::mat4& projection() { return m_camData->projection; }
    tga::StagingBuffer& stage() { return m_camStaging; }
    bool isMoved() { return m_isUpdated; }

    float speed = 0.1f;
    float speedBoost = 8;
    float turnSpeed = 30;

private:
    void processInput(float dt, tga::Interface& tgai) {
        float moveSpeed = speed;

        if (tgai.keyDown(m_window, tga::Key::R)) moveSpeed *= speedBoost;

        if (tgai.keyDown(m_window, tga::Key::Left)) { m_yaw += dt * turnSpeed; m_isUpdated = true; }
        if (tgai.keyDown(m_window, tga::Key::Right)) { m_yaw -= dt * turnSpeed; m_isUpdated = true; }
        if (tgai.keyDown(m_window, tga::Key::Up)) { m_pitch -= dt * turnSpeed; m_isUpdated = true; }
        if (tgai.keyDown(m_window, tga::Key::Down)) { m_pitch += dt * turnSpeed; m_isUpdated = true; }

        m_pitch = std::clamp(m_pitch, -89.f, 89.f);

        auto rot = glm::mat3_cast(glm::quat(glm::vec3(-glm::radians(m_pitch), glm::radians(m_yaw), 0.f)));
        m_lookDir = rot * m_front;
        auto r = rot * glm::cross(m_up, m_front);

        if (tgai.keyDown(m_window, tga::Key::W)) { m_position += m_lookDir * dt * moveSpeed; m_isUpdated = true; }
        if (tgai.keyDown(m_window, tga::Key::S)) { m_position -= m_lookDir * dt * moveSpeed; m_isUpdated = true; }

        if (tgai.keyDown(m_window, tga::Key::A)) { m_position += r * dt * moveSpeed; m_isUpdated = true; }
        if (tgai.keyDown(m_window, tga::Key::D)) { m_position -= r * dt * moveSpeed; m_isUpdated = true; }

        if (tgai.keyDown(m_window, tga::Key::Space)) { m_position += m_up * dt * moveSpeed; m_isUpdated = true; }
        if (tgai.keyDown(m_window, tga::Key::Shift_Left)) { m_position -= m_up * dt * moveSpeed; m_isUpdated = true; }
    }

    void updateData() {
        m_camData->projection = glm::perspective_vk(glm::radians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
        m_camData->view = glm::lookAt(m_position, m_position + m_lookDir, m_up);
    }

    tga::Window m_window;
    tga::StagingBuffer m_camStaging;
    Camera *m_camData;
    float m_fov, m_aspectRatio, m_nearPlane, m_farPlane;
    glm::vec3 m_position, m_front, m_up, m_right, m_lookDir;
    float m_pitch = 0, m_yaw = 0;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    float m_mouseSensitivity = 1;
    bool m_isUpdated = false;
};

/* utils */
void loadObj(const std::string& objFilePath, std::vector<Vertex>& vBuffer, std::vector<uint32_t>& iBuffer) {
    uint32_t vInitialSize = vBuffer.size();
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objFilePath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::vector<Vertex> preVertexBuffer;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.position = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                               attrib.vertices[3 * index.vertex_index + 2]};
            vertex.normal = {attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1],
                             attrib.normals[3 * index.normal_index + 2]};
            vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                         1.f - attrib.texcoords[2 * index.texcoord_index + 1]};
            preVertexBuffer.emplace_back(vertex);
        }
    }

    std::unordered_map<Vertex, uint32_t> foundVertices{};

    for (const auto& vertex : preVertexBuffer) {
        if (!foundVertices.count(vertex)) {  // It's a new Vertex
            foundVertices[vertex] = static_cast<uint32_t>(vBuffer.size() - vInitialSize);
            vBuffer.emplace_back(vertex);
        }
        iBuffer.emplace_back(foundVertices[vertex]);
    }
    preVertexBuffer.clear();
}

template <typename T>
std::tuple<T&, tga::StagingBuffer> stagingBufferOfType(tga::Interface& tgai)
{
    auto stagingBuff = tgai.createStagingBuffer({sizeof(T)});
    return {*static_cast<T *>(tgai.getMapping(stagingBuff)), stagingBuff};
}

int main() {
    /* scene */
    std::vector<ModelCPU> modelData = {
        {"quad.obj",
         Transform(glm::vec3(0,0,0), glm::vec3(0), glm::vec3(1)),
         false,
         "default.png"
        },
        {"duck.obj",
        Transform(glm::vec3(0), glm::vec3(0,-90, 0), glm::vec3(0.01)),
        true,
        "duck.png"}
    };
    std::vector<SphereLight> sphereLights = {
        {glm::vec3(-0.3, 0.4, 0.3), glm::vec3(2), 0.1},
        {glm::vec3(0.3, 0.4, 0.3), glm::vec3(0.9,0.75,1), 0.1}
    };

    /* interface */
    tga::Interface tgai{};

    /* window */
    tga::Window window;
    std::pair<uint32_t, uint32_t> resolution;
    {
        resolution = tgai.screenResolution();
        resolution.first /= 4;
        resolution.second /= 4;
        window = tgai.createWindow({resolution.first, resolution.second});
        tgai.setWindowTitle(window, "demo-06");
    }

    /* models */
    std::vector<Model> models;
    std::vector<Triangle> triangles;
    std::vector<tga::Texture> diffuseMaps;
    uint32_t modelCount;

    tga::Buffer modelBuffer;
    tga::Buffer triangleBuffer;
    tga::Buffer modelCountBuffer;
    {
        // init model info
        uint32_t offset = 0;
        uint32_t stride = 0;
        modelCount = modelData.size();
        models.reserve(modelCount);

        uint32_t diffuseMapId = 0;
        // load default texture
        // diffuseMaps.reserve(modelCount + 1); // +1 for the default diffuse map
        diffuseMaps.emplace_back(tga::loadTexture(resourcePath("textures/default.png"), tga::Format::r8g8b8a8_srgb, tga::SamplerMode::nearest, tga::AddressMode::repeat, tgai, true));
        diffuseMapId++;

        // load models
        std::cout << "Loading the triangles...\n";
        for (int modelIndex = 0; modelIndex < modelCount; modelIndex++) {
            auto& model = modelData[modelIndex];
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            loadObj(resourcePath(std::format("models/{0}", model.name)), vertices, indices);
            
            // load texture
            uint32_t modelDiffuseMapId = 0;
            if(model.diffuseMapName != "") {
                diffuseMaps.emplace_back(tga::loadTexture(resourcePath(std::format("textures/{}",model.diffuseMapName)), tga::Format::r8g8b8a8_srgb, tga::SamplerMode::linear, tgai, true));
                modelDiffuseMapId = diffuseMapId;
                diffuseMapId++;
            }

            for(auto& vertex: vertices) {
                vertex.position = (glm::vec3)(model.transform.transform * glm::vec4(vertex.position, 1));
                vertex.normal = (glm::vec3)(model.transform.transform * glm::vec4(vertex.normal, 1));
            }

            // create triangles out of vertex and index buffers (TODO: can also use index and vertex buffer in compute shader directly, check the performance. Current approach uses more data.)
            uint32_t triangleCount = indices.size() / 3;
            triangles.reserve(triangleCount);
            offset += stride;
            stride = triangleCount;
            for (size_t t = 0; t < triangleCount; t++) {
                // Create a triangle using the corresponding vertices
                Triangle triangle;
                uint32_t i = t*3;
                triangle.v0 = vertices[indices[i]].position;
                triangle.v1 = vertices[indices[i+1]].position;
                triangle.v2 = vertices[indices[i+2]].position;
                triangle.uv0 = vertices[indices[i]].uv;
                triangle.uv1 = vertices[indices[i+1]].uv;
                triangle.uv2 = vertices[indices[i+2]].uv;
                triangle.normal = glm::normalize(glm::cross(triangle.v0 - triangle.v1, triangle.v2 - triangle.v1));

                if(model.invertNormals) triangle.normal *= -1.0f;

                // Add the triangle to the vector
                triangles.emplace_back(std::move(triangle));
            }

            models.emplace_back(
                offset,
                stride,
                modelDiffuseMapId,
                model.diffuseColor
            );
        }

        // create triangle buffer
        {
            size_t size = sizeof(Triangle) * triangles.size();
            tga::StagingBuffer stage = tgai.createStagingBuffer({size, tga::memoryAccess(triangles)});
            triangleBuffer = tgai.createBuffer({tga::BufferUsage::storage, size, stage});
            tgai.free(stage);
        }

        // create model buffer
        {
            size_t size = sizeof(Model) * models.size();
            tga::StagingBuffer stage = tgai.createStagingBuffer({size, tga::memoryAccess(models)});
            modelBuffer = tgai.createBuffer({tga::BufferUsage::storage, size, stage});
            tgai.free(stage);
        }

        // create model count buffer
        {
            tga::StagingBuffer stage = tgai.createStagingBuffer({sizeof(uint32_t), reinterpret_cast<uint8_t *>(std::addressof(modelCount))});
            modelCountBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(uint32_t), stage});
            tgai.free(stage);
        }

    }

    /* camera */
    std::unique_ptr<CameraController> camera;
    tga::Buffer cameraBuffer;
    tga::Buffer cameraPrevBuffer;
    auto[cameraPrev, cameraPrevStage] = stagingBufferOfType<Camera>(tgai);
    {
        camera = std::make_unique<CameraController>(window, 50, resolution.first / float(resolution.second), 0.1f, 30.f, glm::vec3(0, 0.3f, 0.6f), glm::vec3{0, 0, -1}, glm::vec3{0, 1, 0}, tgai);

        cameraBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(Camera), camera->stage()});
        cameraPrevBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(Camera), cameraPrevStage});
    }

    /* light */
    tga::Buffer sphereLightBuffer;
    tga::Buffer sphereLightCountBuffer;
    {
        /* create sphere light buffer */
        {
            tga::StagingBuffer stage = tgai.createStagingBuffer({sizeof(SphereLight) * sphereLights.size(), tga::memoryAccess(sphereLights)});
            sphereLightBuffer = tgai.createBuffer({tga::BufferUsage::storage, sizeof(SphereLight) * sphereLights.size(), stage});
            tgai.free(stage);
        }
    
        /* create sphere light count buffer */
        {
            uint32_t sphereLightCount = sphereLights.size();
            tga::StagingBuffer stage = tgai.createStagingBuffer({sizeof(uint32_t), reinterpret_cast<uint8_t *>(std::addressof(sphereLightCount))});
            sphereLightCountBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(uint32_t), stage});
            tgai.free(stage);
        }
    }

    /* skybox */
    tga::Texture skyboxTexture;
    {
        /*
        tga::HDRImage hdri = tga::loadHDRImage(resourcePath("textures/skybox.hdr"),false);
        std::vector<float> rgba;
        rgba.reserve(hdri.width*hdri.height*4);
        for (size_t i = 0; i < hdri.data.size(); i += 3) {
            rgba.push_back(hdri.data[i]);       // Red
            rgba.push_back(hdri.data[i + 1]);   // Green
            rgba.push_back(hdri.data[i + 2]);   // Blue
            rgba.push_back(1.0f);               // Alpha
        }

        tga::StagingBuffer stage = tgai.createStagingBuffer({sizeof(float)*hdri.data.size(),
        tga::memoryAccess(hdri.data)}); skyboxTexture = tgai.createTexture( {hdri.width, hdri.height,
            tga::Format::r16g16b16a16_sfloat,
            tga::SamplerMode::nearest,
            tga::AddressMode::clampBorder,
            tga::TextureType::_2D,
            1,
            stage}
        );
        */
    }

    /* textures */
    tga::Texture traceStateTexture0, traceStateTexture1, traceStateTexture2, traceStateTexture3; 
    tga::Texture randomNoiseTexture; 
    {
        traceStateTexture0 = tgai.createTexture({resolution.first, resolution.second, tga::Format::r16g16b16a16_sfloat});
        traceStateTexture1 = tgai.createTexture({resolution.first, resolution.second, tga::Format::r16g16b16a16_sfloat});
        traceStateTexture2 = tgai.createTexture({resolution.first, resolution.second, tga::Format::r16g16b16a16_sfloat});
        traceStateTexture3 = tgai.createTexture({resolution.first, resolution.second, tga::Format::r16g16b16a16_sfloat});
        randomNoiseTexture = tga::loadTexture(resourcePath("textures/random-noise.png"), tga::Format::r8_srgb, tga::SamplerMode::linear, tga::AddressMode::repeat, tgai, true);
    }
    
    /* random number */
    auto[randomUVOffset, randomUVOffsetStage] = stagingBufferOfType<glm::vec3>(tgai);
    tga::Buffer randomUVOffsetBuffer;
    {
        randomUVOffset.x = 0;
        randomUVOffset.y = 0;
        randomUVOffsetBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(glm::vec3), randomUVOffsetStage});
    }
    
    /* shaders */
    tga::Shader resetComputeShader;
    tga::Shader cameraUpdateComputeShader;
    tga::Shader rayTracerComputeShader;
    tga::Shader vertexShader;
    tga::Shader fragmentShader;
    {
        resetComputeShader = tga::loadShader(shaderPath("reset_comp.spv"), tga::ShaderType::compute, tgai);
        cameraUpdateComputeShader = tga::loadShader(shaderPath("camera-update_comp.spv"), tga::ShaderType::compute, tgai);
        rayTracerComputeShader = tga::loadShader(shaderPath("ray-tracer_comp.spv"), tga::ShaderType::compute, tgai);
        
        vertexShader = tga::loadShader(shaderPath("image_vert.spv"), tga::ShaderType::vertex, tgai);
        fragmentShader = tga::loadShader(shaderPath("image_frag.spv"), tga::ShaderType::fragment, tgai);
    }

    /* passes */
    tga::ComputePass resetPass;
    tga::ComputePass cameraUpdatePass;
    tga::ComputePass rayTracingPass;
    tga::RenderPass imagePass;
    tga::InputSet resetPassInputSet;
    tga::InputSet cameraUpdatePassInputSet;
    tga::InputSet rayTracingPassInputSet;
    tga::InputSet imagePassInputSet;
    {
        /* scene reset compute pass */
        {
            tga::InputLayout inputLayout({{{
                // S0
                tga::BindingType::storageImage,  // B0: trace state texture 0
                tga::BindingType::storageImage,  // B1: trace state texture 1
                tga::BindingType::storageImage,  // B2: trace state texture 2
                tga::BindingType::storageImage   // B3: trace state texture 3
            }}});
            resetPass = tgai.createComputePass({resetComputeShader, inputLayout});
            resetPassInputSet = tgai.createInputSet({resetPass, 
                                                    {{traceStateTexture0, 0},
                                                     {traceStateTexture1, 1},
                                                     {traceStateTexture2, 2},
                                                     {traceStateTexture2, 3}
                                                    }, 0});
        }

        /* camera update compute pass */
        {
            tga::InputLayout inputLayout({{{
                // S0
                tga::BindingType::uniformBuffer,    // B0: model count buffer
                tga::BindingType::storageBuffer,    // B1: model buffer
                tga::BindingType::uniformBuffer,    // B2: sphere light count buffer
                tga::BindingType::storageBuffer,    // B3: sphere light buffer
                tga::BindingType::storageBuffer,    // B4: triangle buffer
                tga::BindingType::uniformBuffer,    // B5: current camera buffer
                tga::BindingType::uniformBuffer,    // B6: prev camera buffer
                tga::BindingType::storageImage,     // B7: trace state texture 0
                tga::BindingType::storageImage,     // B8: trace state texture 1
                tga::BindingType::storageImage,     // B9: trace state texture 2
                tga::BindingType::storageImage      // B10: trace state texture 3
            }}});
            
            cameraUpdatePass = tgai.createComputePass({cameraUpdateComputeShader, inputLayout});
            cameraUpdatePassInputSet = tgai.createInputSet({cameraUpdatePass, {
                {modelCountBuffer,       0},
                {modelBuffer,            1},
                {sphereLightCountBuffer, 2},
                {sphereLightBuffer,      3},
                {triangleBuffer,         4},
                {cameraBuffer,           5},
                {cameraPrevBuffer,       6},
                {traceStateTexture0,     7},
                {traceStateTexture1,     8},
                {traceStateTexture2,     9},
                {traceStateTexture3,    10},
            }, 0});
        }

        /* ray tracing compute pass */
        {
            tga::InputLayout inputLayout({{{
                // S0
                tga::BindingType::uniformBuffer,                            // B0: model count buffer
                tga::BindingType::storageBuffer,                            // B1: model buffer
                tga::BindingType::uniformBuffer,                            // B2: sphere light count buffer
                tga::BindingType::storageBuffer,                            // B3: sphere light buffer
                tga::BindingType::storageBuffer,                            // B4: triangle buffer
                {tga::BindingType::sampler, (uint32_t)diffuseMaps.size()},  // B5: diffuse maps
                tga::BindingType::uniformBuffer,                            // B6: current camera buffer
                tga::BindingType::storageImage,                             // B7: trace state texture 0
                tga::BindingType::storageImage,                             // B8: trace state texture 1
                tga::BindingType::storageImage,                             // B9: trace state texture 2
                tga::BindingType::storageImage,                             // B10: trace state texture 3
                tga::BindingType::sampler,                                  // B11: random noise
                tga::BindingType::uniformBuffer                             // B12: random vec3 buffer
            }}});

            std::vector<tga::Binding> bindings = {
                {modelCountBuffer,        0},
                {modelBuffer,             1},
                {sphereLightCountBuffer,  2},
                {sphereLightBuffer,       3},
                {triangleBuffer,          4},
                // slot 5 is reserved for diffuse maps
                {cameraBuffer,            6},
                {traceStateTexture0,      7},
                {traceStateTexture1,      8},
                {traceStateTexture2,      9},
                {traceStateTexture3,      10},
                {randomNoiseTexture,      11},
                {randomUVOffsetBuffer,    12},
            };

            for(uint32_t i = 0; i < diffuseMaps.size(); i++) {
                bindings.push_back({diffuseMaps[i], 5, i});
            }

            rayTracingPass = tgai.createComputePass({rayTracerComputeShader, inputLayout});
            rayTracingPassInputSet = tgai.createInputSet({rayTracingPass, bindings, 0});                                      
        }

        /* image render pass */
        {
            tga::InputLayout inputLayout({{{tga::BindingType::sampler, tga::BindingType::sampler}}});
            imagePass = tgai.createRenderPass(tga::RenderPassInfo{vertexShader, fragmentShader, window}
                                                  .setClearOperations(tga::ClearOperation::color)
                                                  .setInputLayout(inputLayout));
            imagePassInputSet = tgai.createInputSet({imagePass, {{traceStateTexture0, 0}, {traceStateTexture3, 1}}, 0});
        }
    }

    /* helper to init scene info texture */
    auto resetSceneTexture = [&]() -> void {
        constexpr auto workGroupSize = 32;
        constexpr auto workGroupMinus1 = workGroupSize - 1;
        auto cmd = tga::CommandRecorder(tgai)
                       .bufferUpload(camera->stage(), cameraBuffer, sizeof(Camera))
                       .setComputePass(resetPass)
                       .bindInputSet(resetPassInputSet)
                       .dispatch((resolution.first + workGroupSize - 1) / workGroupSize, (resolution.second + workGroupSize - 1) / workGroupSize, 1)
                       .endRecording();
        tgai.execute(cmd);
        tgai.waitForCompletion(cmd);
    };

    /* helper to use on camera update */
    auto onCameraUpdate = [&](float deltaTime) -> void {
        // update camera data
        cameraPrev.view = camera->view(); // store the prev view for motion vector
        cameraPrev.projection = camera->projection(); // store the prev projection (TODO: currently not used, may remove it)
        camera->update(deltaTime, tgai);
        if(!camera->isMoved()) return;
        
        // upload camera data
        auto cmd = tga::CommandRecorder(tgai)
                    .bufferUpload(camera->stage(), cameraBuffer, sizeof(Camera)) // upload current camera data
                    .bufferUpload(cameraPrevStage, cameraPrevBuffer, sizeof(Camera)) // upload prev camera data
                    .endRecording();
        tgai.execute(cmd);
        tgai.waitForCompletion(cmd); // make sure camera data is updated properly before the motion vector calculation

        // transfer info between frames
        constexpr auto workGroupSize = 32;
        cmd = tga::CommandRecorder(tgai)
                       .setComputePass(cameraUpdatePass)
                       .bindInputSet(cameraUpdatePassInputSet)
                       .dispatch((resolution.first + workGroupSize - 1) / workGroupSize, (resolution.second + workGroupSize - 1) / workGroupSize, 1)
                       .endRecording();
        tgai.execute(cmd);
        tgai.waitForCompletion(cmd);
    };

    /* stop watches */
    double deltaTime = 1.0 / 60.0f;
    std::chrono::steady_clock::time_point ts_mainLoop;

    /* main loop */
    tga::CommandBuffer cmdBuffer{};
    uint32_t nextFrame;
    resetSceneTexture();
    while (!tgai.windowShouldClose(window)) {
        /* on before render */
        {
            ts_mainLoop = std::chrono::steady_clock::now();

            // camera update
            onCameraUpdate(deltaTime);

            randomUVOffset = glm::vec3(glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f));
            nextFrame = tgai.nextFrame(window);
        }

        /* ray tracing compute pass*/
        {
            constexpr auto workGroupSize = 32;
            auto cmd = tga::CommandRecorder(tgai)
                           .bufferUpload(randomUVOffsetStage, randomUVOffsetBuffer, sizeof(glm::vec3))
                           .setComputePass(rayTracingPass)
                           .bindInputSet(rayTracingPassInputSet)
                           .dispatch((resolution.first + workGroupSize - 1) / workGroupSize, (resolution.second + workGroupSize - 1) / workGroupSize, 1)
                           .endRecording();
            tgai.execute(cmd);
            tgai.waitForCompletion(cmd);
        }

        /* image render pass*/
        {
            cmdBuffer = tga::CommandRecorder{tgai, cmdBuffer}
                            .setRenderPass(imagePass, nextFrame, {0.2, 0.2, 0.2, 1.})
                            .bindInputSet(imagePassInputSet)
                            .draw(3, 0)
                            .endRecording();
            // Execute commands and show the result
            tgai.execute(cmdBuffer);
            tgai.present(window, nextFrame);
        }

        /* on after render */
        {
            //std::this_thread::sleep_for(std::chrono::milliseconds(10));
            deltaTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - ts_mainLoop).count();
        }

        /* log */
        {
            printf("\033c");
            std::cout << std::format("fps: {:.0f}\n", 1/deltaTime);
        }
    }

    return 0;
}