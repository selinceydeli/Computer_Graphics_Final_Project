#version 410

// Per-vertex attributes
in vec2 texCoords;

// Data to pass to fragment shader
out vec2 fragCoords;

void main() {
    fragCoords = texCoords;
}