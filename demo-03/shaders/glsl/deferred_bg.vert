#version 450
#extension GL_ARB_separate_shader_objects : enable

#define INSTANCE_COUNT 128

// input
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

// uniform
layout(set = 0, binding = 0) uniform CamData{
    mat4 view;
    mat4 projection;
} cam;

layout(set = 1, binding = 1) uniform ObjectData{
    mat4 model[INSTANCE_COUNT];
} object;

// out
layout(location = 0) out FragData{
    vec3 positionWorld;
    vec2 uv;
    vec3 normal;
} frag; 

void main()
{
    vec4 positionWorld = object.model[gl_InstanceIndex] * vec4(pos.x, pos.y, pos.z, 1.0);
    gl_Position = cam.projection * cam.view * positionWorld;

    frag.positionWorld = positionWorld.xyz;
    frag.uv = uv;
    frag.normal = mat3(transpose(inverse(object.model[gl_InstanceIndex]))) * normal;
}