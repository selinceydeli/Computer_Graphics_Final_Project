#version 410

in vec2 fragCoords;
in vec4 particleColor;
out vec4 color;

void main() {
    color = particleColor;
}
