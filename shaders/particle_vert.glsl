#version 410

// Per-vertex attributes
in vec2 texPos;
in vec2 texCoords;

// Data to pass to fragment shader
out vec2 fragPos;
out vec2 fragCoords;

uniform mat4 mvp;
uniform vec2 offset;

void main() {
    fragPos = texPos;
    fragCoords = texCoords;
    float scale = 0.05;
    gl_Position = mvp * vec4(texPos.x * scale + offset.x, 10, texPos.y * scale + offset.y, 1.0); 
}
