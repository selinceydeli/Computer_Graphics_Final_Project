#version 410 core

// Global variables for lighting calculations.
//uniform vec3 viewPos;
uniform sampler2D texLight;

uniform int applyTexture; 

// scene uniforms
uniform mat4 lightMVP;
uniform vec3 lightPos;

// config uniforms, use these to control the shader from UI
uniform int peelingMode;
uniform int lightMode;
uniform int lightColorMode;

// PCF sampling for smooth shadow
const int pcfSamples = 2;

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

    /*
    // Check if the shadow map coordinates are outside [0, 1] range
    if (fragLightCoord.x < 0.0 || fragLightCoord.x > 1.0 || fragLightCoord.y < 0.0 || fragLightCoord.y > 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);    // Set the fragment color to black
        return;
    }
    */

    float fragLightDepth = fragLightCoord.z;
    vec2 shadowMapCoord = clamp(fragLightCoord.xy, 0.0f, 1.0f);

    float bias = 0.001;

    if (applyTexture == 1) {
        vec3 sampledColor = texture(texLight, shadowMapCoord + vec2(bias)).rgb;
        outColor = vec4(sampledColor, 1.0); 
    }
}