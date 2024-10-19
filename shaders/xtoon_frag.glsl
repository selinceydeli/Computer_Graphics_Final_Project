#version 410

// Global variables for lighting calculations
//uniform vec3 viewPos;
uniform vec3 lightPos;          
uniform vec3 cameraPos;     
uniform vec3 kd;    
uniform vec3 ks;            
uniform float shininess;  
uniform vec3 lightColor;
uniform sampler2D texToon;      

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
    vec3 H = normalize(L + V);                                          // Halfway vector

    // Setting the horizontal component
    float lambertian = max(dot(N, L), 0.0);
    vec3 diffuse = kd * lambertian * lightColor;                                // Diffuse component
    vec3 specular = ks * pow(max(dot(N, H), 0.0), shininess) * lightColor;      // Specular component

    float brightness = clamp(length(diffuse + specular), 0.0, 1.0);

    // Setting the vertical component
    float distance = length(fragPos - cameraPos);                       // World-space distance between viewer and fragment
    
    // Scaling the distance to [0, 1]
    float minDistance = 1, maxDistance = 10;
    float scaledDistance = clamp((distance - minDistance) / (maxDistance - minDistance), 0.0, 1.0);

    outColor = texture(texToon, vec2(brightness, scaledDistance));
}