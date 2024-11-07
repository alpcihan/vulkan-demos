#version 450

// input 
layout(location = 0) in FragData{
    vec3 position;
    vec2 uv;
    vec3 normal;
}fragData;

// uniform
layout(set = 0, binding = 1) uniform LightData{
    vec3 position;
    vec3 color;
} lightData;

layout(set = 1, binding = 1) uniform sampler2D tex;

// output
layout(location = 0) out vec4 color;

vec3 ambient = vec3(0.02);
float distTH = 5.0f;

void main()
{
    // base color
    vec3 c = texture(tex, fragData.uv).xyz;

    // ambient + diffuse
    vec3 norm = normalize(fragData.normal);
    vec3 lightDir = normalize(lightData.position - fragData.position); 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightData.color; 

    float dist = distance(fragData.position, lightData.position) / distTH;
    dist = clamp(dist, 0, 1);
    
    c = (ambient + diffuse) * c;

    // specular
    vec3 viewPos = vec3(0.0,0.0,-10.0); // TODO: get it from model matrix
    vec3 viewDir = normalize(viewPos - fragData.position);
    vec3 reflectDir = reflect(-lightDir, norm); 
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    c += spec;

    color = vec4(c,1.0f);
}
