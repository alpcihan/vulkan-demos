#version 450
#extension GL_ARB_separate_shader_objects : enable

// input 
layout(location = 0) in FragData{ 
    vec3 positionWorld;
    vec2 uv;
    vec3 normal;
} frag; 

layout(set = 1, binding = 0) uniform sampler2D tex;

// output
layout (location = 0) out vec4 cc1;
layout (location = 1) out vec4 cc2;
layout (location = 2) out vec4 cc3;

void main()
{
    vec3 pos = frag.positionWorld.xyz;
    vec3 normal = normalize(frag.normal);
    vec3 color = texture(tex,frag.uv).rgb;
    
    cc1 = vec4(pos, 1);
    cc2 = vec4(normal, 1);
    cc3 = vec4(color, 0);
}
