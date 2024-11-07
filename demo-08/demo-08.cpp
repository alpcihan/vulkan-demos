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

struct BVHNode {
    alignas(16) glm::vec3 aabbMin;
    alignas(16) glm::vec3 aabbMax;
    uint32_t leftChild;
    uint32_t rightChild;
    uint32_t firstTriangleIdx;
    uint32_t triangleCount;
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
          m_camData(static_cast<Camera *>(tgai.getMapping(m_camStaging))), m_fov(fov), m_aspectRatio(aspectRatio),
          m_nearPlane(nearPlane), m_farPlane(farPlane), m_position(position), m_front(front), m_up(up),
          m_right(glm::cross(up, front)) {
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

    float speed = 1;
    float speedBoost = 8;
    float turnSpeed = 90;

private:
    void processInput(float dt, tga::Interface& tgai) {
        float moveSpeed = speed;

        if (tgai.keyDown(m_window, tga::Key::R)) moveSpeed *= speedBoost;

        if (tgai.keyDown(m_window, tga::Key::Left)) {
            m_yaw += dt * turnSpeed;
            m_isUpdated = true;
        }
        if (tgai.keyDown(m_window, tga::Key::Right)) {
            m_yaw -= dt * turnSpeed;
            m_isUpdated = true;
        }
        if (tgai.keyDown(m_window, tga::Key::Up)) {
            m_pitch -= dt * turnSpeed;
            m_isUpdated = true;
        }
        if (tgai.keyDown(m_window, tga::Key::Down)) {
            m_pitch += dt * turnSpeed;
            m_isUpdated = true;
        }

        m_pitch = std::clamp(m_pitch, -89.f, 89.f);

        auto rot = glm::mat3_cast(glm::quat(glm::vec3(-glm::radians(m_pitch), glm::radians(m_yaw), 0.f)));
        m_lookDir = rot * m_front;
        auto r = rot * glm::cross(m_up, m_front);

        if (tgai.keyDown(m_window, tga::Key::W)) {
            m_position += m_lookDir * dt * moveSpeed;
            m_isUpdated = true;
        }
        if (tgai.keyDown(m_window, tga::Key::S)) {
            m_position -= m_lookDir * dt * moveSpeed;
            m_isUpdated = true;
        }

        if (tgai.keyDown(m_window, tga::Key::A)) {
            m_position += r * dt * moveSpeed;
            m_isUpdated = true;
        }
        if (tgai.keyDown(m_window, tga::Key::D)) {
            m_position -= r * dt * moveSpeed;
            m_isUpdated = true;
        }

        if (tgai.keyDown(m_window, tga::Key::Space)) {
            m_position += m_up * dt * moveSpeed;
            m_isUpdated = true;
        }
        if (tgai.keyDown(m_window, tga::Key::Shift_Left)) {
            m_position -= m_up * dt * moveSpeed;
            m_isUpdated = true;
        }
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
template <typename T>
std::tuple<T&, tga::StagingBuffer> stagingBufferOfType(tga::Interface& tgai) {
    auto stagingBuff = tgai.createStagingBuffer({sizeof(T)});
    return {*static_cast<T *>(tgai.getMapping(stagingBuff)), stagingBuff};
}

std::vector<BVHNode> bvh;
uint32_t nodesUsed;
std::vector<Triangle> triangles;
std::vector<uint32_t> triIdx;

void updateNodeBounds(const uint32_t nodeIdx) {
    BVHNode& node = bvh[nodeIdx];
    node.aabbMin = glm::vec3(1e30f);
    node.aabbMax = glm::vec3(-1e30f);
    for (uint32_t first = node.firstTriangleIdx, i = 0; i < node.triangleCount; i++) {
        uint32_t leafTriIdx = triIdx[first + i];
        Triangle& leafTri = triangles[leafTriIdx];
        node.aabbMin = glm::min(node.aabbMin, leafTri.v0);
        node.aabbMin = glm::min(node.aabbMin, leafTri.v1);
        node.aabbMin = glm::min(node.aabbMin, leafTri.v2);
        node.aabbMax = glm::max(node.aabbMax, leafTri.v0);
        node.aabbMax = glm::max(node.aabbMax, leafTri.v1);
        node.aabbMax = glm::max(node.aabbMax, leafTri.v2);
    }
}

void subdivide(const uint32_t nodeIdx) {
    // terminate recursion
    BVHNode& node = bvh[nodeIdx];
    if (node.triangleCount <= 2) return;
    // determine split axis and position
    glm::vec3 extent = node.aabbMax - node.aabbMin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
    // in-place partition
    int i = node.firstTriangleIdx;
    int j = i + node.triangleCount - 1;
    while (i <= j) {
        const Triangle& tri = triangles[triIdx[i]];
        const float centroid = (tri.v0[axis] + tri.v1[axis] + tri.v2[axis]) / 3;
        if (centroid < splitPos)
            i++;
        else
            std::swap(triIdx[i], triIdx[j--]);
    }
    
    int leftCount = i - node.firstTriangleIdx;
    if (leftCount == 0 || leftCount == node.triangleCount) return;
    
    int leftChildIdx = nodesUsed++;
    int rightChildIdx = nodesUsed++;
    bvh[leftChildIdx].firstTriangleIdx = node.firstTriangleIdx;
    bvh[leftChildIdx].triangleCount = leftCount;
    bvh[rightChildIdx].firstTriangleIdx = i;
    bvh[rightChildIdx].triangleCount = node.triangleCount - leftCount;
    node.leftChild = leftChildIdx;
    node.triangleCount = 0;
    updateNodeBounds(leftChildIdx);
    updateNodeBounds(rightChildIdx);
    
    subdivide(leftChildIdx);
    subdivide(rightChildIdx);
}

int main() {
    /* scene */
    std::vector<ModelCPU> modelData = {
        {"dragon.obj", Transform(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0.08)), true},
    };
    std::vector<SphereLight> sphereLights = {{glm::vec3(1, 0.4, 0.5), glm::vec3(5), 0.2}};

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
        tgai.setWindowTitle(window, "demo-08");
    }

    /* models */
    std::vector<tga::Texture> diffuseMaps;
    std::vector<tga::Vertex> vertices;
    std::vector<uint32_t> indices;
    tga::Buffer vertexBuffer, indexBuffer, triangleBuffer;
    {
        // init model info
        uint32_t offset = 0;
        uint32_t stride = 0;

        uint32_t diffuseMapId = 0;
        // load default texture
        // diffuseMaps.reserve(modelCount + 1); // +1 for the default diffuse map
        diffuseMaps.emplace_back(tga::loadTexture(resourcePath("textures/default.png"), tga::Format::r8g8b8a8_srgb,
                                                  tga::SamplerMode::nearest, tga::AddressMode::repeat, tgai, true));
        diffuseMapId++;

        // load models
        glm::vec3 mx(std::numeric_limits<float>::min()), mn(std::numeric_limits<float>::max());
        std::cout << "Loading the triangles...\n";
        for (int modelIndex = 0; modelIndex < modelData.size(); modelIndex++) {
            auto& model = modelData[modelIndex];
            tga::Obj obj = tga::loadObj(resourcePath(std::format("models/{0}", model.name)));

            // load texture
            uint32_t modelDiffuseMapId = 0;
            if (model.diffuseMapName != "") {
                diffuseMaps.emplace_back(
                    tga::loadTexture(resourcePath(std::format("textures/{}", model.diffuseMapName)),
                                     tga::Format::r8g8b8a8_srgb, tga::SamplerMode::linear, tgai, true));
                modelDiffuseMapId = diffuseMapId;
                diffuseMapId++;
            }

            for (auto& vertex : obj.vertexBuffer) {
                vertex.position = (glm::vec3)(model.transform.transform * glm::vec4(vertex.position, 1));
                vertex.normal = (glm::vec3)(model.transform.transform * glm::vec4(vertex.normal, 1));
            }

            // create triangles out of vertex and index buffers (TODO: can also use index and vertex buffer in compute
            // shader directly, check the performance. Current approach uses more data.)
            uint32_t triangleCount = obj.indexBuffer.size() / 3;
            triangles.reserve(triangleCount);
            offset += stride;
            stride = triangleCount;
            for (size_t t = 0; t < triangleCount; t++) {
                // Create a triangle using the corresponding vertices
                Triangle triangle;
                uint32_t i = t * 3;
                triangle.v0 = obj.vertexBuffer[obj.indexBuffer[i]].position;
                triangle.v1 = obj.vertexBuffer[obj.indexBuffer[i + 1]].position;
                triangle.v2 = obj.vertexBuffer[obj.indexBuffer[i + 2]].position;
                triangle.uv0 = obj.vertexBuffer[obj.indexBuffer[i]].uv;
                triangle.uv1 = obj.vertexBuffer[obj.indexBuffer[i + 1]].uv;
                triangle.uv2 = obj.vertexBuffer[obj.indexBuffer[i + 2]].uv;
                triangle.normal = glm::normalize(glm::cross(triangle.v0 - triangle.v1, triangle.v2 - triangle.v1));

                if (model.invertNormals) triangle.normal *= -1.0f;

                // Add the triangle to the vector
                triangles.emplace_back(std::move(triangle));
            }

            vertices.insert(
                vertices.end(), 
                std::make_move_iterator(obj.vertexBuffer.begin()),
                std::make_move_iterator(obj.vertexBuffer.end()));
            
            indices.insert(
                indices.end(), 
                std::make_move_iterator(obj.indexBuffer.begin()),
                std::make_move_iterator(obj.indexBuffer.end()));
        }

        // create triangle buffer
        {
            size_t size = sizeof(Triangle) * triangles.size();
            tga::StagingBuffer stage = tgai.createStagingBuffer({size, tga::memoryAccess(triangles)});
            triangleBuffer = tgai.createBuffer({tga::BufferUsage::storage, size, stage});
            tgai.free(stage);
        }

        // create vertex buffer
        {
            size_t size = sizeof(tga::Vertex) * vertices.size();
            tga::StagingBuffer stage = tgai.createStagingBuffer({size, tga::memoryAccess(vertices)});
            vertexBuffer = tgai.createBuffer({tga::BufferUsage::accelerationStructureBuildInput | tga::BufferUsage::storage, size, stage});
            tgai.free(stage);
        }

        // create index buffer
        {
            size_t size = sizeof(uint32_t) * indices.size();
            tga::StagingBuffer stage = tgai.createStagingBuffer({size, tga::memoryAccess(indices)});
            indexBuffer = tgai.createBuffer({tga::BufferUsage::accelerationStructureBuildInput | tga::BufferUsage::storage, size, stage});
            tgai.free(stage);
        }
    }

    /* bvh */
    tga::Buffer bvhBuffer;
    tga::Buffer triIdxBuffer;
    {
        uint32_t N = triangles.size();
        bvh.resize(2 * N - 1);

        triIdx.reserve(N);
        for (int i = 0; i < N; i++) {
            triIdx.push_back(i);
        }

        BVHNode& root = bvh[0];
        nodesUsed = 1;
        root.leftChild = 0;
        root.triangleCount = N;
        updateNodeBounds(0);
        subdivide(0);

        // create bvh buffer
        {
            size_t size = sizeof(BVHNode) * bvh.size();
            tga::StagingBuffer stage = tgai.createStagingBuffer({size, tga::memoryAccess(bvh)});
            bvhBuffer = tgai.createBuffer({tga::BufferUsage::storage, size, stage});
            tgai.free(stage);
        }

        // create triangle idx buffer
        {
            size_t size = sizeof(uint32_t) * triIdx.size();
            tga::StagingBuffer stage = tgai.createStagingBuffer({size, tga::memoryAccess(triIdx)});
            triIdxBuffer = tgai.createBuffer({tga::BufferUsage::storage, size, stage});
            tgai.free(stage);
        }
    }

    /* acceleration structure */
    tga::ext::TopLevelAccelerationStructure tlas;
    {   
        const auto& t = modelData[0].transform.transform;
        tga::ext::TransformMatrix transform = {{
                {t[0].x, t[0].y, t[0].z, t[0].w},
                {t[1].x, t[1].y, t[1].z, t[1].w},
                {t[2].x, t[2].y, t[2].z, t[2].w}
        }};

        const std::vector<tga::ext::AccelerationStructureInstanceInfo> blasList = {
            {
                tgai.createBottomLevelAccelerationStructure({
                            vertexBuffer,
                            indexBuffer,
                            sizeof(tga::Vertex),
                            tga::Format::r32g32b32_sfloat,
                            (uint32_t)indices.size(),
                            0,
                            0
                }),
                transform // TODO: do not copy this
            },
        };

        tlas = tgai.createTopLevelAccelerationStructure({blasList}); // TODO: do not copy blasList
    }

    /* camera */
    std::unique_ptr<CameraController> camera;
    tga::Buffer cameraBuffer;
    tga::Buffer cameraPrevBuffer;
    auto [cameraPrev, cameraPrevStage] = stagingBufferOfType<Camera>(tgai);
    {
        camera =
            std::make_unique<CameraController>(window, 50, resolution.first / float(resolution.second), 0.1f, 30.f,
                                               glm::vec3(0, 0.3f, 0.6f), glm::vec3{0, 0, -1}, glm::vec3{0, 1, 0}, tgai);

        cameraBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(Camera), camera->stage()});
        cameraPrevBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(Camera), cameraPrevStage});
    }

    /* light */
    tga::Buffer sphereLightBuffer;
    tga::Buffer sphereLightCountBuffer;
    {
        /* create sphere light buffer */
        {
            tga::StagingBuffer stage =
                tgai.createStagingBuffer({sizeof(SphereLight) * sphereLights.size(), tga::memoryAccess(sphereLights)});
            sphereLightBuffer =
                tgai.createBuffer({tga::BufferUsage::storage, sizeof(SphereLight) * sphereLights.size(), stage});
            tgai.free(stage);
        }

        /* create sphere light count buffer */
        {
            uint32_t sphereLightCount = sphereLights.size();
            tga::StagingBuffer stage = tgai.createStagingBuffer(
                {sizeof(uint32_t), reinterpret_cast<uint8_t *>(std::addressof(sphereLightCount))});
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
    tga::Texture traceStateTexture;
    tga::Texture randomNoiseTexture;
    {
        traceStateTexture = tgai.createTexture({resolution.first, resolution.second, tga::Format::r16g16b16a16_sfloat});
        randomNoiseTexture = tga::loadTexture(resourcePath("textures/random-noise.png"), tga::Format::r8_srgb,
                                              tga::SamplerMode::linear, tga::AddressMode::repeat, tgai, true);
    }

    /* random number */
    auto [randomUVOffset, randomUVOffsetStage] = stagingBufferOfType<glm::vec3>(tgai);
    tga::Buffer randomUVOffsetBuffer;
    {
        randomUVOffset.x = 0;
        randomUVOffset.y = 0;
        randomUVOffsetBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(glm::vec3), randomUVOffsetStage});
    }

    /* shaders */
    tga::Shader resetComputeShader;
    tga::Shader rayTracerComputeShader;
    tga::Shader vertexShader;
    tga::Shader fragmentShader;
    {
        resetComputeShader = tga::loadShader(shaderPath("reset_comp.spv"), tga::ShaderType::compute, tgai);
        rayTracerComputeShader = tga::loadShader(shaderPath("ray-tracer_comp.spv"), tga::ShaderType::compute, tgai);

        vertexShader = tga::loadShader(shaderPath("image_vert.spv"), tga::ShaderType::vertex, tgai);
        fragmentShader = tga::loadShader(shaderPath("image_frag.spv"), tga::ShaderType::fragment, tgai);
    }

    /* passes */
    tga::ComputePass resetPass;
    tga::ComputePass rayTracingPass;
    tga::RenderPass imagePass;
    tga::InputSet resetPassInputSet;
    tga::InputSet rayTracingPassInputSet;
    tga::InputSet imagePassInputSet;
    {
        /* scene reset compute pass */
        {
            tga::InputLayout inputLayout({{{
                // S0
                tga::BindingType::storageImage,  // B0: trace state texture
            }}});
            resetPass = tgai.createComputePass({resetComputeShader, inputLayout});
            resetPassInputSet = tgai.createInputSet({resetPass,
                                                     {
                                                         {traceStateTexture, 0},
                                                     },
                                                     0});
        }

        /* ray tracing compute pass */
        {
            tga::InputLayout inputLayout({{{
                // S0
                tga::BindingType::uniformBuffer,                            // B0: sphere light count buffer
                tga::BindingType::storageBuffer,                            // B1: sphere light buffer
                tga::BindingType::storageBuffer,                            // B2: triangle buffer
                tga::BindingType::uniformBuffer,                            // B3: current camera buffer
                tga::BindingType::storageImage,                             // B4: trace state texture 0
                tga::BindingType::sampler,                                  // B5: random noise
                tga::BindingType::uniformBuffer,                            // B6: random vec3 buffer
                tga::BindingType::storageBuffer,                            // B7: bvh buffer
                tga::BindingType::storageBuffer,                            // B8: triangle idx buffer
                tga::BindingType::accelerationStructure,                    // B9: acceleration structure
                tga::BindingType::storageBuffer,                            // B10: vertices
                tga::BindingType::storageBuffer,                            // B11: indices
            }}});

            std::vector<tga::Binding> bindings = {{sphereLightCountBuffer,  0},
                                                  {sphereLightBuffer,       1},
                                                  {triangleBuffer,          2},
                                                  {cameraBuffer,            3},
                                                  {traceStateTexture,       4},
                                                  {randomNoiseTexture,      5},
                                                  {randomUVOffsetBuffer,    6},
                                                  {bvhBuffer,               7},
                                                  {triIdxBuffer,            8},
                                                  {tlas,                    9},
                                                  {vertexBuffer,           10},
                                                  {indexBuffer,            11},
                                                };

            rayTracingPass = tgai.createComputePass({rayTracerComputeShader, inputLayout});
            rayTracingPassInputSet = tgai.createInputSet({rayTracingPass, bindings, 0});
        }

        /* image render pass */
        {
            tga::InputLayout inputLayout({{{tga::BindingType::sampler}}});
            imagePass = tgai.createRenderPass(tga::RenderPassInfo{vertexShader, fragmentShader, window}
                                                  .setClearOperations(tga::ClearOperation::color)
                                                  .setInputLayout(inputLayout));
            imagePassInputSet = tgai.createInputSet({imagePass, {{traceStateTexture, 0}}, 0});
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
                       .dispatch((resolution.first + workGroupSize - 1) / workGroupSize,
                                 (resolution.second + workGroupSize - 1) / workGroupSize, 1)
                       .endRecording();
        tgai.execute(cmd);
        tgai.waitForCompletion(cmd);
    };

    /* helper to use on camera update */
    auto onCameraUpdate = [&](float deltaTime) -> void {
        // update camera data
        cameraPrev.view = camera->view();  // store the prev view for motion vector
        cameraPrev.projection =
            camera->projection();  // store the prev projection (TODO: currently not used, may remove it)
        camera->update(deltaTime, tgai);
        if (!camera->isMoved()) return;
        resetSceneTexture();
    };

    /* timers */
    double deltaTime = 1.0 / 60.0f;
    double logTimer = 0;
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

            randomUVOffset =
                glm::vec3(glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.0f));
            nextFrame = tgai.nextFrame(window);
        }

        /* ray tracing compute pass*/
        {
            constexpr auto workGroupSize = 32;
            auto cmd = tga::CommandRecorder(tgai)
                           .bufferUpload(randomUVOffsetStage, randomUVOffsetBuffer, sizeof(glm::vec3))
                           .setComputePass(rayTracingPass)
                           .bindInputSet(rayTracingPassInputSet)
                           .dispatch((resolution.first + workGroupSize - 1) / workGroupSize,
                                     (resolution.second + workGroupSize - 1) / workGroupSize, 1)
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
            //std::this_thread::sleep_for(std::chrono::milliseconds(15));
            deltaTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - ts_mainLoop).count();
            {
                logTimer += deltaTime;
                if (logTimer > 0.1) {
                    printf("\033c");
                    std::cout << std::format("fps: {:.0f}\n", 1/deltaTime);
                    logTimer = 0;
                }
            }
        }
    }

    return 0;
}