#include "gpro/gpro.hpp"

#define INSTANCE_COUNT 50
#define LIGHT_COUNT 128
#define SCREEN_SCALE 0.4

glm::vec3 rnd3() {
    // Generate random values for each component in the range [0, 1) and floor them
    float x = glm::linearRand(0.0f, 1.0f);
    float y = glm::linearRand(0.0f, 1.0f);
    float z = glm::linearRand(0.0f, 1.0f);

    // Return the resulting glm::vec3
    return glm::vec3(x, y, z);
}

struct Screen
{
    uint32_t w, h;
};

int main()
{
    // Open the interface
    tga::Interface tgai{};

    // Create window with the size of the screen
    auto [screenResX, screenResY] = tgai.screenResolution();
    Screen screen {screenResX * SCREEN_SCALE, screenResY * SCREEN_SCALE};
    tga::Buffer resolutionBuffer = gpro::util::createBuffer(tga::BufferUsage::uniform, sizeof(Screen), toui8(ref(screen)), tgai);

    tga::Window window = tgai.createWindow({screen.w, screen.h});
    tgai.setWindowTitle(window, "demo 3");

    // Camera
    gpro::CameraController camera(tgai, window, 90, screen.w / float(screen.h), 0.1f, 30000.f, glm::vec3(0, 2.5, -3),glm::vec3{0, -0.5, 1}, glm::vec3{0, 1, 0});
    camera.speed = 7;
    camera.speedBoost = 8;
    camera.turnSpeed = 50;

    // Single CommandBuffer that will be reused every frame
    tga::CommandBuffer cmdBuffer{};

    // Load meshes
    std::vector<gpro::Mesh> meshes{
        gpro::Mesh(tgai, gpro::resourcePath("models/transporter.obj")),
        gpro::Mesh(tgai, gpro::resourcePath("models/juf.obj")),
    };

    // Load diffuse maps
    std::vector<tga::Texture> diffuseMaps{
        tga::loadTexture(gpro::resourcePath("textures/transporter.png"), tga::Format::r8g8b8a8_srgb,
                         tga::SamplerMode::linear, tgai, true),
        tga::loadTexture(gpro::resourcePath("textures/juf.png"), tga::Format::r8g8b8a8_srgb, tga::SamplerMode::linear,
                         tgai, true)};

    // Transforms
    std::vector<gpro::Transform> obj1Transforms;
    for(int i = 0; i < INSTANCE_COUNT/4; i++) obj1Transforms.push_back({glm::vec3(1, 0, i*5), glm::vec3(0, -25, 0), glm::vec3(0.006)});
    for(int i = 0; i <= INSTANCE_COUNT/4; i++) obj1Transforms.push_back({glm::vec3(6, 0, i*5), glm::vec3(0, -25, 0), glm::vec3(0.006)});
    
    std::vector<gpro::Transform> obj2Transforms;
    for(int i = 0; i < INSTANCE_COUNT/4; i++) obj2Transforms.push_back({glm::vec3(-2.5, 0, i*5), glm::vec3(0, 150, 0), glm::vec3(0.011)});
    for(int i = 0; i <= INSTANCE_COUNT/4; i++) obj2Transforms.push_back({glm::vec3(-5.5, 0, i*5), glm::vec3(0, 150, 0), glm::vec3(0.011)});

    std::vector<tga::Buffer> objTransformBuffers{
        gpro::util::createBuffer(tga::BufferUsage::uniform, sizeof(gpro::Transform) * obj1Transforms.size(), toui8(obj1Transforms.data()), tgai),
        gpro::util::createBuffer(tga::BufferUsage::uniform, sizeof(gpro::Transform) * obj2Transforms.size(), toui8(obj2Transforms.data()), tgai)};

    // Camera
    tga::Buffer camBuffer = tgai.createBuffer({tga::BufferUsage::uniform, sizeof(gpro::CamData), camera.Data()});

    // Lights
    std::vector<gpro::Light> lights;
    for(int i = 0; i < LIGHT_COUNT/4; i++) lights.push_back({glm::vec3(1, 6, i*5 - 5), rnd3()});
    for(int i = 0; i < LIGHT_COUNT/4; i++) lights.push_back({glm::vec3(6, 6, i*5 - 5), rnd3()});
    for(int i = 0; i < LIGHT_COUNT/4; i++) lights.push_back({glm::vec3(-2.5, 6, i*5 - 5), rnd3()});
    for(int i = 0; i < LIGHT_COUNT/4; i++) lights.push_back({glm::vec3(-5.5, 6, i*5 - 5), rnd3()});
    
    tga::Buffer lightBuffer = gpro::util::createBuffer(tga::BufferUsage::uniform, sizeof(gpro::Light) * lights.size(),
                                                       toui8(lights.data()), tgai);

    // gbuffer
    std::vector<tga::Texture> gbuffer{
        tgai.createTexture({screenResX, screenResY, tga::Format::r16g16b16a16_sfloat}), // color
        tgai.createTexture({screenResX, screenResY, tga::Format::r16g16b16a16_sfloat}), // pos
        tgai.createTexture({screenResX, screenResY, tga::Format::r16g16b16a16_sfloat}), // normal 
    };

    // Renderpass 1
    tga::Shader vs = tga::loadShader(gpro::shaderPath("deferred_bg_vert.spv"), tga::ShaderType::vertex, tgai);
    tga::Shader fs = tga::loadShader(gpro::shaderPath("deferred_bg_frag.spv"), tga::ShaderType::fragment, tgai);

    tga::InputLayout inputLayoutBG({
        {{{tga::BindingType::uniformBuffer}}},                                  // Set = 0: Camera Data
        {{{tga::BindingType::sampler}, {tga::BindingType::uniformBuffer}}},     // Set = 1: Diffuse Map, Object Data
    });

    tga::RenderPass renderPassBG = tgai.createRenderPass(tga::RenderPassInfo{
        vs,
        fs,
        {},
        {},
        inputLayoutBG,
        {tga::ClearOperation::all},
        {tga::CompareOperation::less},
        {tga::FrontFace::counterclockwise, tga::CullMode::back}}
            .setVertexLayout(gpro::Mesh::getVertexLayout())
            .setRenderTarget(gbuffer));

    // Renderpass 2
    tga::Shader vsFG = tga::loadShader(gpro::shaderPath("deferred_fg_vert.spv"), tga::ShaderType::vertex, tgai);
    tga::Shader fsFG = tga::loadShader(gpro::shaderPath("deferred_fg_frag.spv"), tga::ShaderType::fragment, tgai);
    
    tga::InputLayout inputLayoutFG({
        {{{tga::BindingType::sampler, 1}, {tga::BindingType::sampler, 1}, {tga::BindingType::sampler, 1}}},      // Set = 0: G-Buffer
        {{{tga::BindingType::uniformBuffer}, {tga::BindingType::uniformBuffer}}},                                // Set = 1: Camera, Lights
    });
    
    tga::Texture rt = tgai.createTexture({screenResX, screenResY, tga::Format::r16g16b16a16_sfloat});

    tga::RenderPass renderPassFG =
        tgai.createRenderPass(tga::RenderPassInfo{vsFG, fsFG, window}
            .setInputLayout(inputLayoutFG)
            .setRenderTarget(std::vector<tga::Texture>{rt})
        );

    // Renderpass 3
    tga::Shader vsP = tga::loadShader(gpro::shaderPath("deferred_post_vert.spv"), tga::ShaderType::vertex, tgai);
    tga::Shader fsP = tga::loadShader(gpro::shaderPath("deferred_post_frag.spv"), tga::ShaderType::fragment, tgai);

    tga::InputLayout inputLayoutP({
        {{{tga::BindingType::sampler, 1}, {tga::BindingType::uniformBuffer}}}
    });

    tga::RenderPass renderPassP =
        tgai.createRenderPass(tga::RenderPassInfo{vsP, fsP, window}
            .setInputLayout(inputLayoutP)
        );

    // input sets
    std::vector<tga::InputSet> inputSetsBG{
        tgai.createInputSet({renderPassBG, {{camBuffer, 0}}, 0}),                                       // (s:0, b: 0)      camera
        tgai.createInputSet({renderPassBG, {{diffuseMaps[0], 0}, {objTransformBuffers[0], 1}}, 1}),     // (s:1, b: 0,1)    obj1, transform + diffuse
        tgai.createInputSet({renderPassBG, {{diffuseMaps[1], 0}, {objTransformBuffers[1], 1}}, 1})      // (s:1, b: 0,1)    obj2, transform + diffuse
    };

    std::vector<tga::InputSet> inputSetsFG{
        tgai.createInputSet({renderPassFG, {{gbuffer[0], 0}, {gbuffer[1], 1}, {gbuffer[2], 2}}, 0}),    // (s:0, b: 0,1,2)  gbuffer
        tgai.createInputSet({renderPassFG, {{camBuffer, 0}, {lightBuffer, 1}}, 1})                      // (s:1, b: 0, 1)   camera + lights
    };

    std::vector<tga::InputSet> inputSetsP{
        tgai.createInputSet({renderPassP, {{rt, 0}, {resolutionBuffer, 1}}, 0}),                         // (s:0, b: 0)  render texture, screen resolution
    };

    double deltaTime = 1. / 60.;
    double smoothedDeltaTime = 0;
    size_t deltaTimeCount = 0;


    while (!tgai.windowShouldClose(window)) {
        auto ts = std::chrono::steady_clock::now();

        auto nextFrame = tgai.nextFrame(window);
        cmdBuffer = tga::CommandRecorder{tgai, cmdBuffer}  
                        .bufferUpload(camera.Data(), camBuffer, sizeof(gpro::CamData))
                        // first render pass
                        .setRenderPass(renderPassBG, 0, {0, 0, 0, 1})
                        .bindInputSet(inputSetsBG[0])                   // bind camera data

                        // obj1
                        .bindInputSet(inputSetsBG[1])                   // bind obj1 data
                        .bindVertexBuffer(meshes[0].getVertexBuffer())
                        .bindIndexBuffer(meshes[0].getIndexBuffer())
                        .drawIndexed(meshes[0].getIndexCount(), 0, 0, INSTANCE_COUNT)

                        // obj2
                        .bindInputSet(inputSetsBG[2])                   // bind obj2 data
                        .bindVertexBuffer(meshes[1].getVertexBuffer())
                        .bindIndexBuffer(meshes[1].getIndexBuffer())
                        .drawIndexed(meshes[1].getIndexCount(), 0, 0, INSTANCE_COUNT)

                        // second render pass
                        .setRenderPass(renderPassFG, 0)
                        .bindInputSet(inputSetsFG[0])
                        .bindInputSet(inputSetsFG[1])
                        .draw(3, 0)

                        // post processing
                        .setRenderPass(renderPassP, nextFrame)
                        .bindInputSet(inputSetsP[0])
                        .draw(3, 0)

                        .endRecording();
        
        camera.update(deltaTime);

        // Execute commands and show the result
        tgai.execute(cmdBuffer);
        tgai.present(window, nextFrame);  

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto tn = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration<double>(tn - ts).count(); 

        smoothedDeltaTime += deltaTime;
        deltaTimeCount++;
    }

    return 0;
}