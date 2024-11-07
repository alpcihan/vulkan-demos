#version 460
#extension GL_EXT_nonuniform_qualifier: enable

// inputs
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

// bindings
layout(set = 0, binding = 0) uniform Camera {
    mat4 mat_view;
    mat4 mat_projection;
};

layout(set = 0, binding = 3) uniform Time {
    float time;
};

layout(set = 2, binding = 0) readonly buffer Models {
    mat4 models[];
};

layout(set = 2, binding = 1) readonly buffer InstanceIdToMeshIDMap{
    uint instanceIdToMeshIDMap[];
};

// output
layout(location = 0) out Frag{
    vec3 position;
    vec2 uv;
    vec3 normal;
    flat uint drawID;
}frag;

void main() {
    // vertex world pos
    mat4 model = models[gl_InstanceIndex];
    uint meshID = instanceIdToMeshIDMap[gl_InstanceIndex]; 
    vec3 worldPos = (model * vec4(position, 1.0)).xyz;
    
    gl_Position = mat_projection * mat_view * vec4(worldPos,1);
    
    // pass fragment data
    frag.position = worldPos.xyz;
    frag.uv = uv;
    frag.normal = mat3(transpose(inverse(model))) * normal;
    frag.drawID = meshID;
}
