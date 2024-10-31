#version 410

out vec4 outColor;

in vec2 fragPos;
in vec2 fragCoords;

uniform sampler2D screenTexture;
uniform float brightnessThreshold = 1.0;
uniform float blurSize = 1.0 / 300.0;
uniform float glowIntensity = 0.5;


void main()
{
    // outColor = texture(screenTexture, fragCoords);
    vec3 color = texture(screenTexture, fragCoords).rgb;
    float brightness = dot(color, vec3(0.299, 0.587, 0.114));
    vec3 brightColor = brightness > brightnessThreshold ? color : vec3(0.0);

    vec3 blurColor = vec3(0.0);
    for (int i = -4; i <= 4; i++) {
        float weight = 0.1; 
        blurColor += texture(screenTexture, fragCoords + vec2(i * blurSize, 0.0)).rgb * weight;
        blurColor += texture(screenTexture, fragCoords + vec2(0.0, i * blurSize)).rgb * weight;
    }

    outColor = vec4(color + glowIntensity * blurColor, 1.0);
}