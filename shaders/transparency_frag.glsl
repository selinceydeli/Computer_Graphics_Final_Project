#version 410

// Global variables for lighting calculations.
//uniform vec3 viewPos;
uniform sampler2D texShadow;

// scene uniforms
uniform mat4 mvp;
uniform vec3 lightPos;
uniform vec3 cameraPos;

// Output for on-screen color.
out vec4 outColor;

// Interpolated output data from vertex shader.
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 N = normalize(fragNormal);                             // Normal
    vec3 L = normalize(cameraPos - fragPos);                     
    float diffuse = abs(dot(N, L));

    vec4 fragLightCoord = mvp * vec4(fragPos, 1.0);
    fragLightCoord.xyz /= fragLightCoord.w;
    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;

    float fragLightDepth = fragLightCoord.z;
    vec2 shadowMapCoord = clamp(fragLightCoord.xy, 0.0f, 1.0f);

    float bias = 0.001;

    // Define circular region for peeling
    float distFromCenter = length(shadowMapCoord - vec2(0.5, 0.5));
    float regionRadius = 0.2; 

    if (distFromCenter < regionRadius) {
        float shadowMapDepth = texture(texShadow, shadowMapCoord).x;
        bool inShadow = fragLightDepth - bias > shadowMapDepth;
        if (!inShadow) discard;
    }

    //vec3 lighting = spotlightFactor * shadow * diffuse * color;  
    vec3 lighting = vec3(diffuse);  
    outColor = vec4(lighting, 1.0); 

}