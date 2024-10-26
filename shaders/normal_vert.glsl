#version 410

in vec3 pos;
in vec3 normal;
in vec3 tangent;
in vec3 bitangent;

out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

uniform mat4 mvp;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    gl_Position = mvp * vec4(pos, 1.0);

    mat3 normalMatrix = mat3(1.0); // use Identity Matrix for simplicity here
    vec3 T = normalize(normalMatrix * tangent);
    vec3 B = normalize(normalMatrix * bitangent);
    vec3 N = normalize(normalMatrix * normal);
    mat3 TBN = mat3(T, B, N);

    TangentLightPos = TBN * lightPos;
    TangentViewPos = TBN * viewPos;
    TangentFragPos = TBN * pos;
}