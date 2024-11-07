#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 uv;

layout(set = 0, binding = 0) uniform sampler2D trace;

layout (location = 0) out vec4 color;

void main() {
    vec3 c = texture(trace, uv).xyz;
    color = vec4(c, 1);
}
