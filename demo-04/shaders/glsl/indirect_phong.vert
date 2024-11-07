#version 460
#extension GL_EXT_nonuniform_qualifier: enable

// input
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

// uniform
layout(set = 0, binding = 0) uniform Camera{
    mat4 mat_view;
    mat4 mat_projection;
};

layout(set = 0, binding = 2) uniform Time{
    float time;
};

layout(set = 2, binding = 0) uniform Object{
    mat4 mat_models[100];
};

layout(set = 2, binding = 1) uniform Pattern{
    vec3 patterns[100];
};

// output
layout(location = 0) out Frag{
    vec3 position;
    vec2 uv;
    vec3 normal;
    flat uint drawID;
}frag;

float rand(vec3 co){
    return fract(sin(dot(co, vec3(12.9898, 78.233, 99.11))) * 43758.5453);
}

void main()
{
    mat4 model = mat_models[gl_DrawID];
    vec3 pattern = patterns[gl_DrawID];

    vec3 worldPos = (model * vec4(position, 1.0)).xyz;
    
    // add random movement
    worldPos += gl_InstanceIndex * pattern;
    vec3 vertical = vec3(pattern.y, pattern.x, 1);
    float c = rand(vec3(gl_DrawID + gl_InstanceIndex + 0.3, gl_InstanceIndex - gl_DrawID + 0.123, 1)) + 0.1; 
    float x = sin(time * c * 2) * c * 3;
    float y = cos(time * c * 2) * c * 3;
    float z = sin(cos(time * c * 4) * c * 2) * c * 3;
    worldPos += vertical * vec3(x, y*c, z);
    worldPos += pattern * vec3(z, x, y);

    gl_Position = mat_projection * mat_view * vec4(worldPos,1);

    frag.position = worldPos.xyz;
    frag.uv = uv;
    frag.normal = mat3(transpose(inverse(model))) * normal;
    frag.drawID = gl_DrawID;
}
