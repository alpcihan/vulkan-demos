#version 450
#extension GL_ARB_separate_shader_objects : enable

#define LIGHT_COUNT 128

struct Light {
    vec3 position;
    vec3 color;
};

// g-buffer
layout(set = 0, binding = 0) uniform sampler2D cc1;
layout(set = 0, binding = 1) uniform sampler2D cc2;
layout(set = 0, binding = 2) uniform sampler2D cc3;

// uniform
layout(set = 1, binding = 0) uniform CamData{
    mat4 view;
    mat4 projection;
} camera;

layout(set = 1, binding = 1) uniform LightData{
    Light lights[LIGHT_COUNT];
};

// in
layout (location = 0) in vec2 uv;

// out
layout (location = 0) out vec4 color;

void main() 
{
    // read g-buffer
    vec3 position   = texture(cc1,uv).xyz;
    vec3 normal     = texture(cc2,uv).xyz;
    vec4 col4       = texture(cc3,uv);

    vec3 col = col4.xyz;

    // Blinn Phong Illumination model
    vec3 N = normalize(normal);

    // The ambient term is replaced by a gradient
    float NdS = clamp(0.5*N.y+0.5, 0.0, 1.0);
    vec3 skyColor = vec3(0.5, 0.5, 0.5);
    vec3 ambient = NdS * skyColor;

    vec3 camPos = camera.view[3].xyz;
    vec3 V = normalize(camPos-position);
    
    // The specularity is normalized to approximate energy conservation
    float n = 128.;

    for(int i = 0; i < LIGHT_COUNT; i++)
    {
        Light light = lights[i];

        vec3 L = normalize(light.position);
        float NdL = max(dot(N,L),0);

        vec3 H = normalize(V+L);

        float spec = pow(max(dot(N,H),0),n);
        spec *= (n+2)/(4*(2-exp2(-n*0.5)));

        // The decompose the light data
        vec3 lightCol = light.color;

        // make distance dependent
        float coeff = pow((distance(position, light.position)+0.001) / 6, 10);
        coeff = 1 / coeff;

        col += col*lightCol*NdL * coeff;
        col += lightCol*spec * coeff * 0.5;
    }

    col += col*skyColor*NdS;
    color = vec4(col,col4.w);
}
