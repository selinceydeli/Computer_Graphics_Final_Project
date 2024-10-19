#version 410

// Global variables for lighting calculations
//uniform vec3 viewPos;
uniform vec3 lightPos;    
uniform vec3 cameraPos;            
uniform float shininess;  
uniform float toonSpecularThreshold;
uniform vec3 ks;
uniform vec3 lightColor;      

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

// Toon specular applies a threshold to the Blinn-Phong specular highlights
void main()
{
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(lightPos - fragPos);                             // Light direction 
    vec3 V = normalize(cameraPos - fragPos);                            // View direction
    vec3 H = normalize(L + V);                                          // Halfway vector H = (L + V) / ||L + V||

    vec3 blinnSpecular = ks * pow(max(dot(N, H), 0.0), shininess) * lightColor;

    // Compute grayscale intensity for specular highlights
    float specularGrayscale = 0.299 * blinnSpecular.r + 0.587 * blinnSpecular.g + 0.114 * blinnSpecular.b;

    // Compute intensity with length method
    float specularLength = length(blinnSpecular);      

    if (specularLength >= toonSpecularThreshold) {
        outColor = vec4(1.0, 1.0, 1.0, 1.0);        // Set pixel to white
    } 
    else {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);        // Set pixel to black
    }
}