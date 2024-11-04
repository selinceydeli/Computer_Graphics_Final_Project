#version 410

in vec2 fragCoords;

out vec4 color;

uniform vec4 particleColor;

void main() {
    // color = vec4(1.0, 0.0, 0.0, 1.0);
    color = particleColor;
}
