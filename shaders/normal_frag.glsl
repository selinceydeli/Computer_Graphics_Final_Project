#version 410

in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

out vec4 FragColor;

uniform sampler2D texNormal;

void main()
{
    // Sample the normal from the normal map
    vec3 normal = texture(texNormal, gl_FragCoord.xy).rgb;
    normal = normalize(normal * 2.0 - 1.0); // Convert to range [-1, 1]

    // Lighting calculations
    vec3 lightDir = normalize(TangentLightPos - TangentFragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    // Sample the diffuse color
    vec3 diffuse = diff * vec3(1.0);

    // Compute final color
    FragColor = vec4(diffuse, 1.0);
}