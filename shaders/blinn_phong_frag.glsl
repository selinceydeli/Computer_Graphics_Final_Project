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
    vec3 L = normalize(lightPos - fragPos);                             // Light direction 
    vec3 V = normalize(cameraPos - fragPos);                            // View direction
    vec3 H = normalize(L + V);                                          // Halfway vector H = (L + V) / ||L + V||

    vec3 blinnSpecular = ks * pow(max(dot(N, H), 0.0), shininess) * lightColor;   // Specular component using Blinn-Phong model

    outColor = vec4(blinnSpecular, 1.0);
}