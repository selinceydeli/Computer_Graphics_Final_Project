#version 410

in vec3 TangentLightPos;
// in vec3 TangentViewPos;
in vec3 TangentFragPos;
in vec2 FragTexCoord;
in mat3 TBN;

out vec4 outColor;

uniform sampler2D texNormal;
uniform sampler2D texMat;
// uniform vec3 kd;
// uniform vec3 lightColor;

void main()
{
    vec3 normal = texture(texNormal, FragTexCoord).xyz;
    normal = normal * 2.0 - 1.0; // Convert to range [-1, 1]
    // normal = normalize(TBN * normal);

    vec3 lightDir = normalize(TangentLightPos - TangentFragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuseColor = texture(texMat, FragTexCoord).xyz;
    vec3 ambient = 0.1 * diffuseColor;

    outColor = vec4(diff * diffuseColor + ambient, 1.0);
    // outColor = vec4(diff * vec3(1.0), 1.0);
    // outColor = vec4(diffuseColor, 1.0);
    // outColor = vec4(normal * 0.5 + 0.5, 1.0);
}