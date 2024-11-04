#version 410 core 

layout (location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 posEye;
out vec3 nEye;

void main() {
    posEye = vec3(model * vec4(position, 1.0));
    // nEye = vec3(view * model * vec4(normal, 0.0));
    nEye = mat3(model) *normal;
    gl_Position = projection * view * model * vec4(position, 1.0);
}







// #version 410 core 
// layout (location = 0) in vec3 aPos;

// out vec3 texCoords;

// uniform mat4 projection;
// uniform mat4 view; 

// void main() {
//     vec4 pos = projection * view * vec4(aPos, 1.0f);
//     gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);
//     texCoords = vec3(aPos.x, aPos.y, -aPos.z); 
// }

// #version 410

// layout(location = 0) in vec3 aPos;
// layout(location = 1) in vec3 normal;

// out vec3 anormal;
// out vec3 pos;
// out vec3 texCoords;

// // Model/view/projection matrix
// uniform mat4 projection;
// uniform mat4 model; // Add this to get the model matrix

// // Per-vertex attributes
// uniform mat4 view; // World-space position

// void main() {
//     anormal = mat3(transpose(inverse(model))) *normal;
//     pos = vec3(model * vec4(aPos, 1.0));
//    texCoords = vec3(model * vec4(aPos, 1.0));
//    //vec4 pos = projection * view * model * vec4(aPos, 1.0);
//    gl_Position = projection * view * model * vec4(aPos, 1.0);
// }

