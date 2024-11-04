// Fragment Shader
#version 410 core 

in vec3 posEye;
in vec3 nEye;

uniform samplerCube skybox;
uniform mat4 view;
uniform vec3 cameraPos;

out vec4 fragColor; 

void main() {
    vec3 I = normalize(posEye - cameraPos);
    
    vec3 N = normalize(nEye);

    vec3 reflected = reflect(I, N);
    //reflected = vec3(inverse(view) * vec4(reflected, 0.0));

    fragColor = texture(skybox, reflected);
}





//#version 410 core

// uniform samplerCube cubemapTexture; // Environment map

// // Passes from the vertex shader
// in vec3 anormal;
// in vec3 pos;

// out vec4 outColor;
// uniform vec3 cameraPos;

// void main() {
//     // Output color (you can blend this with your diffuse/specular lighting calculations)
//     vec3 i = normalize(pos - cameraPos);
//     vec3 r = reflect(i, normalize(anormal));
//     outColor = vec4(texture(cubemapTexture, r).rgb, 1.0);
// }

// #version 410 core 

// out vec4 fragColor;

// in vec3 texCoords; 

// uniform samplerCube skybox; 

// void main() {
//     fragColor = texture(skybox, texCoords);
// }