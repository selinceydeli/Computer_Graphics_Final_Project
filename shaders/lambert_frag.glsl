#version 410

// Global variables for lighting calculations
//uniform vec3 viewPos;
uniform vec3 lightPos;   
uniform vec3 lightColor;  
uniform vec3 kd;          

uniform sampler2D texLight;
uniform int applyTexture; 

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 N = normalize(fragNormal);                             // Normal
    vec3 L = normalize(lightPos - fragPos);                     // Light direction

    float lambertian = max(dot(N, L), 0.0);                                 
    vec3 diffuse = kd * lambertian * lightColor;
    outColor = vec4(diffuse, 1.0);                              
}