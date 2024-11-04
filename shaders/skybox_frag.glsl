#version 410 core 

out vec4 fragColor;

in vec3 texCoords; 

uniform samplerCube skybox; 

void main() {
    fragColor = texture(skybox, texCoords);
}

// #version 410 core 

// out vec4 fragColor;

// in vec3 texCoords; 
// in vec3 Normal;

// uniform samplerCube skybox; 
// uniform vec3 cameraPos;
// uniform mat4 view;

// void main() {
//     // vec3 I = normalize(texCoords - cameraPos);
//     // vec3 reflectionDir = reflect(I, normalize(Normal));
//     // vec3 color = texture(skybox, reflectionDir).rgb;
//     // fragColor = vec4(color, 1.0);
//     //fragColor = texture(skybox, texCoords);

//     vec3 incident_eye = normalize(texCoords - cameraPos);
//     vec3 normall = normalize(Normal);

//     vec3 reflected = reflect(incident_eye, normall);
//     // convert from eye to world space
//     //reflected = vec3(inverse(view) * vec4(reflected, 0.0));

//     fragColor = texture(skybox, reflected);
// }