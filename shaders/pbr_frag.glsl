#version 410

#define PI 3.1415926
#define eps 1e-5

// Global variables for lighting calculations
//uniform vec3 viewPos;
// const int NUM_LIGHTS = 3;
// uniform vec3 lightPos[NUM_LIGHTS];
// uniform vec3 lightColor[NUM_LIGHTS];
uniform vec3 lightPos;    
uniform vec3 lightColor;  
uniform vec3 cameraPos;  
uniform vec3 albedo;  // Base color, we use this instead of Ks 
uniform float roughness;
uniform float metallic;
uniform float intensity;
// uniform sampler2D texNormal;

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

// Modified Schilick Fresnel, ref. UE
vec3 fresnel(vec3 V, vec3 H) {
    vec3 F0 = mix(vec3(0.04), vec3(0.95, 0.82, 0.52), metallic); // When it's plastic, F0=0.04; else we use gold's F0
    float VdotH = max(dot(V, H), 0.0);
    float powIndex = (-5.55473 * VdotH - 6.98316) * VdotH;
    return F0 + (1 - F0) * pow(2, powIndex);
}

float NDF(vec3 N, vec3 H, float roughness) {
    float alpha = roughness * roughness;
    float NdotH = max(dot(N, H), 0.0);
    float nom = alpha * alpha;
    float denom = PI * pow(NdotH * NdotH * (alpha * alpha - 1) + 1, 2);
    return nom / denom;
}

float G1(vec3 N, vec3 variable, float roughness) {
    float k = pow(roughness + 1, 2) / 8;
    float NdotVariable = max(dot(N, variable), 0.0);
    return NdotVariable / (NdotVariable * (1 - k) + k);
}

float GGX(vec3 N, vec3 L, vec3 V, float roughness) {
    return G1(N, L, roughness) * G1(N, V, roughness);
}

void main()
{
    // Compute basic vectors
    vec3 N = normalize(fragNormal);
    // vec3 N = texture(texNormal, gl_FragCoord.xy).xyz;
    vec3 V = normalize(cameraPos - fragPos);                            // View direction
    vec3 L = normalize(lightPos - fragPos);                             // Light direction 
    vec3 H = normalize(L + V);                                          // Halfway vector H = (L + V) / ||L + V||
    N = dot(N, L) < 0.0 ? -N:N;
    vec3 accumColor = vec3(0.0);

    // Compute radiance
    float dis2Light = distance(lightPos, fragPos);
    float attenuation = clamp(1.0 / dis2Light, 0.2, 1.0);  // Limit the attenuation
    vec3 LightIntensity = lightColor * attenuation * intensity;

    // Cook-Torrance Specular Microfacets Model
    vec3 F = fresnel(V, H);
    float D = NDF(N, H, roughness);
    float G = GGX(N, L, V, roughness);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    vec3 specular = F * D * G / (4 * NdotL * NdotV);

    // Diffuse Light
    vec3 diffuse = albedo / PI;
    accumColor += ((1.0 - metallic) * diffuse + specular) * NdotL * LightIntensity;
    
    // Ambient
    vec3 ambient = vec3(0.1) * albedo;

    // Sum up
    outColor = vec4(ambient + accumColor, 1.0);                       // Multiply the cos value
}