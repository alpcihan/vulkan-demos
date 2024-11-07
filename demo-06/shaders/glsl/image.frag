#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 uv;

layout(set = 0, binding = 0) uniform sampler2D trace0;
layout(set = 0, binding = 1) uniform sampler2D trace3;

layout (location = 0) out vec4 color;

void main() {
    vec3 c = texture(trace0, uv).xyz;
    color = vec4(c, 1);
}
