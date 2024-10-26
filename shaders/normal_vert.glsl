#version 410

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 texCoord;

out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;
out vec2 FragTexCoord;

uniform mat4 mvp;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    mat3 normalMatrix = mat3(1.0); // use Identity Matrix for simplicity here
    vec3 T = normalize(normalMatrix * tangent);
    vec3 B = normalize(normalMatrix * bitangent);
    vec3 N = normalize(normalMatrix * normal);
    mat3 TBN = mat3(T, B, N);

    TangentLightPos = TBN * lightPos;
    TangentViewPos = TBN * viewPos;
    TangentFragPos = TBN * pos;

    FragTexCoord = texCoord;
    gl_Position = mvp * vec4(pos, 1.0);
}