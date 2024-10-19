#version 410 core

// Global variables for lighting calculations.
//uniform vec3 viewPos;
uniform sampler2D texShadow;

uniform int isSpotlight;

// scene uniforms
uniform mat4 lightMVP;
uniform vec3 lightPos;

// Output for on-screen color.
out vec4 outColor;

// Interpolated output data from vertex shader.
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 N = normalize(fragNormal);                             // Normal
    vec3 L = normalize(lightPos - fragPos);                     // Light direction
    float diffuse = abs(dot(N, L));

    vec4 fragLightCoord = lightMVP * vec4(fragPos, 1.0);
    fragLightCoord.xyz /= fragLightCoord.w;
    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;

    float fragLightDepth = fragLightCoord.z;
    vec2 shadowMapCoord = clamp(fragLightCoord.xy, 0.0f, 1.0f);

    float bias = 0.001;

    float distFromCenter = distance(shadowMapCoord, vec2(0.5, 0.5));
    float spotlightFactor = clamp(1.0 - distFromCenter * 2.0, 0.0, 1.0); 
    outColor = vec4(vec3(spotlightFactor), 1.0); 

}