#version 450

// input
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

// uniform
layout(set = 0, binding = 0) uniform CamData{
    mat4 view;
    mat4 projection;
} camData;

layout(set = 1, binding = 0) uniform ObjectData{
    mat4 model;
} objectData;

// output
layout(location = 0) out FragData{
    vec3 position;
    vec2 uv;
    vec3 normal;
}fragData;

void main()
{
    vec4 worldSpace = objectData.model * vec4(pos.x, pos.y, pos.z, 1.0);
    gl_Position = camData.projection * camData.view * worldSpace;

    fragData.position = worldSpace.xyz;
    fragData.uv = uv;
    fragData.normal = mat3(transpose(inverse(objectData.model))) * normal;
}
