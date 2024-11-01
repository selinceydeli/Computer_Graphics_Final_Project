#version 410 core

// Global variables for lighting calculations.
//uniform vec3 viewPos;
uniform sampler2D shadowTex2;

uniform int isSpotlight;
uniform int shadows;
uniform int pcf;

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


float applySampling(float fragLightDepth, vec2 shadowMapCoord, float bias) {

    float shadow = 0.0;

    // Smooth-shadow implementation
    if (pcf == 1) {
        float totalSamples = float((pcfSamples * 2 + 1) * (pcfSamples * 2 + 1));        // Total number of samples
        for (int y = -pcfSamples; y <= pcfSamples; ++y) {
            for (int x = -pcfSamples; x <= pcfSamples; ++x) {
                vec2 offset = vec2(float(x), float(y)) * 0.001;
                float shadowMapDepth = texture(shadowTex2, shadowMapCoord + offset).x; 
                shadow += fragLightDepth - bias > shadowMapDepth ? 0.0 : 1.0;  
            }
        }
        shadow /= totalSamples;
    }

    // Hard-shadow implementation
    else if (shadows == 1) {
        float shadowMapDepth = texture(shadowTex2, shadowMapCoord).x;
        shadow = fragLightDepth - bias > shadowMapDepth ? 0.0 : 1.0;  // Assign 0.f to shadowed fragments, 1.f to illuminated fragments                                                    
    }

    else {
        shadow = 1.0;
    }

    return shadow; 
}

float changeLightMode(vec2 shadowMapCoord) {    
    // Spotlight  
    if (isSpotlight == 1) {
        float distFromCenter = distance(shadowMapCoord, vec2(0.5, 0.5));
        float spotlightFactor = clamp(1.0 - distFromCenter * 2.0, 0.0, 1.0); 
        return spotlightFactor;
    }

    // Normal light mode
    return 1.0f;
}

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

    float shadow = applySampling(fragLightDepth, shadowMapCoord, bias);       // Apply shadow
    float spotlightFactor = changeLightMode(shadowMapCoord);                  // Change light mode

    vec3 lighting = vec3(spotlightFactor * shadow * diffuse);  
    outColor = vec4(lighting, 1.0); 

}