#version 410

out vec4 outColor;

in vec2 fragPos;
in vec2 fragCoords;

uniform sampler2D screenTexture;

void main()
{
    outColor = texture(screenTexture, fragCoords);
    // outColor = vec4(1.0, 1.0, 0.0, 1.0);
}