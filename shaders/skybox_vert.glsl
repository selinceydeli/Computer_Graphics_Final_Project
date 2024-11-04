#version 410 core 
layout (location = 0) in vec3 aPos;

out vec3 texCoords;

uniform mat4 projection;
uniform mat4 view; 

void main() {
    vec4 pos = projection * view * vec4(aPos, 1.0f);
    gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);
    texCoords = vec3(aPos.x, aPos.y, -aPos.z); 
}


// #version 410 core 
// layout (location = 0) in vec3 aPos;
// layout (location = 1) in vec3 normal;

// out vec3 texCoords;
// out vec3 Normal;

// uniform mat4 projection;
// uniform mat4 view; 
// uniform mat4 model;

// void main() {
//     texCoords = vec3(view * model * vec4(aPos, 1.0));
//     Normal = mat3(transpose(inverse(model))) * normal;
//     //Normal = vec3(view * model * vec4(normal, 0.0));
//     gl_Position = projection * view * model * vec4(aPos, 1.0);
//     // vec4 pos = projection * view * vec4(aPos, 1.0f);
//     // gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);
//     // texCoords = vec3(aPos.x, aPos.y, -aPos.z); 
// }