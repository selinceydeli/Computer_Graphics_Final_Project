#version 410

// Global variables for lighting calculations
//uniform vec3 viewPos;
uniform vec3 lightPos;    
uniform vec3 lightColor;  
uniform vec3 cameraPos;            
uniform vec3 ks;          
uniform float shininess;  

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPos);                 // Light direction
    vec3 V = normalize(cameraPos - fragPos);                // View direction
    vec3 R = reflect(-L, N);                                // Reflection vector

    vec3 phongSpecular = ks * pow(max(dot(R, V), 0.0), shininess) * lightColor;   // Specular component using Phong model

    outColor = vec4(phongSpecular, 1.0);
}