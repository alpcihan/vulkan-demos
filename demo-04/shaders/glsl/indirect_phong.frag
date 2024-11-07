#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#define LIGHT_COUNT 1

struct Light {
    vec3 position;
    vec3 color;
};

// input 
layout(location = 0) in Frag{
    vec3 position;
    vec2 uv;
    vec3 normal;
    flat uint drawID;
}frag;

// uniform
layout(set = 0, binding = 0) uniform Camera{
    mat4 mat_view;
    mat4 mat_projection;
};

layout(set = 0, binding = 1) uniform Lights{
    Light lights[LIGHT_COUNT];
};

layout(set = 1, binding = 0) uniform sampler2D diffuseMaps[];

// output
layout(location = 0) out vec4 color;

void main()
{
    // base color
    vec4 col4 = texture(diffuseMaps[frag.drawID], frag.uv);

    vec3 col = col4.xyz;

    // Blinn Phong Illumination model
    vec3 N = normalize(frag.normal);

    // The ambient term is replaced by a gradient
    float NdS = clamp(0.5*N.y+0.5, 0.0, 1.0);
    vec3 skyColor = vec3(0.5, 0.5, 0.5);
    vec3 ambient = NdS * skyColor;

    vec3 camPos = mat_view[3].xyz;
    vec3 V = normalize(camPos-frag.position);
    
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
        float coeff = pow((distance(frag.position, light.position)+0.001) / 6, 10);
        coeff = 1 / coeff;

        col += col*lightCol*NdL * coeff;
        col += lightCol*spec * coeff * 0.5;
    }

    col += col*skyColor*NdS;
    color = vec4(col,col4.w);
}
