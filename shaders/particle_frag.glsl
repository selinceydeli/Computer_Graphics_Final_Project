#version 410

in vec2 fragCoords;

out vec4 color;

uniform vec4 particleColor;
uniform sampler2D texToon;

void main() {
    vec4 texColor = texture(texToon, fragCoords);
    color = particleColor;
}
