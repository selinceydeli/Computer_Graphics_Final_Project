#version 410

// Global variables for lighting calculations
//uniform vec3 viewPos;
uniform vec3 lightPos;   
uniform vec3 lightColor;  
uniform vec3 kd;          

uniform int diffuseMode;
uniform int specularMode;

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

/*
float applyShadow(float fragLightDepth, vec2 shadowMapCoord, float bias) {

    float shadow = 0.0;

    // Hard-shadow implementation
    if (samplingMode == 0) {
        float shadowMapDepth = texture(texShadow, shadowMapCoord).x;
        shadow = fragLightDepth - bias > shadowMapDepth ? 0.0 : 1.0;  // Assign 0.f to shadowed fragments, 1.f to illuminated fragments                                                    
    }

    // Smooth-shadow implementation
    else {
        float totalSamples = float((pcfSamples * 2 + 1) * (pcfSamples * 2 + 1));        // Total number of samples
        for (int y = -pcfSamples; y <= pcfSamples; ++y) {
            for (int x = -pcfSamples; x <= pcfSamples; ++x) {
                vec2 offset = vec2(float(x), float(y)) * 0.001;
                float shadowMapDepth = texture(texShadow, shadowMapCoord + offset).x; 
                shadow += fragLightDepth - bias > shadowMapDepth ? 0.0 : 1.0;  
            }
        }
        shadow /= totalSamples;
    }

    return shadow; 
}
*/

/*
std::array diffuseModes {"Debug", "Lambert Diffuse", "Toon Lighting Diffuse", "Toon X Lighting"};
std::array specularModes {"None", "Phong Specular Lighting", "Blinn-Phong Specular Lighting", "Toon Lighting Specular"};
*/


void main()
{
    vec3 N = normalize(fragNormal);                             // Normal
    vec3 L = normalize(lightPos - fragPos);                     // Light direction

    /*
    if (diffuseMode == 0) {
        // debug
    }
    else if (diffuseMode == 1) {
        // lambert diffuse
    }
    else if (diffuseMode == 2) {
        // toon lighting
    }
    else {
        //toon x lighting
    }
    */

    float lambertian = max(dot(N, L), 0.0);                                 
    vec3 diffuse = kd * lambertian * lightColor;
    outColor = vec4(diffuse, 1.0);                              
}