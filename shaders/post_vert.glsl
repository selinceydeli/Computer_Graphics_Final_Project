#version 410

// Per-vertex attributes
in vec2 texPos;
in vec2 texCoords;

// // Data to pass to fragment shader
out vec2 fragPos;
out vec2 fragCoords;

uniform mat4 mvp;

void main() {
    fragPos = texPos;
    fragCoords = texCoords;
    gl_Position = vec4(texPos.x, texPos.y, 0.0, 1.0);
}