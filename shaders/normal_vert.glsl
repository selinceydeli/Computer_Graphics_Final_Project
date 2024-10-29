#version 410

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 texCoord;

out vec3 TangentLightPos;
// out vec3 TangentViewPos;
out vec3 TangentFragPos;
out vec2 FragTexCoord;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 lightPos;
// uniform vec3 viewPos;

void main()
{
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 B = normalize(normalMatrix * bitangent);
    vec3 N = normalize(normalMatrix * normal);
    TBN = mat3(T, B, N);

    vec3 fragPosWorld = vec3(model * vec4(pos, 1.0));
    TangentLightPos = TBN * lightPos;
    // TangentViewPos = TBN * viewPos;
    TangentFragPos = TBN * fragPosWorld;

    FragTexCoord = texCoord;
    gl_Position = proj * view * vec4(fragPosWorld, 1.0);
}