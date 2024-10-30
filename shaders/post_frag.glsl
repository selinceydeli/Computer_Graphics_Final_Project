#version 410

out vec4 outColor;

// in vec2 fragPos;
// in vec2 fragCoords;

uniform sampler2D screenTexture;

void main()
{
    outColor = vec4(texture(screenTexture, vec2(0.0, 0.0)).xyz, 1.0);
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}