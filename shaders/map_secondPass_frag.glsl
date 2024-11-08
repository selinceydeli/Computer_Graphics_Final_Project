#version 410 core

// Uniforms for lighting and textures
uniform vec3 lightPos;
uniform mat4 lightMVP;
uniform mat4 mvp;

// Minimap texture
uniform sampler2D texMinimap;

// Variables for displaying the minimap
vec2 overlayCenter = vec2(0.9, 0.9);
float overlaySize = 0.3;
vec3 overlayBackgroundColor = vec3(0.2, 0.2, 0.7);

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos;    // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 N = normalize(fragNormal);               
    vec3 L = normalize(lightPos - fragPos);       
    float diffuse = abs(dot(N, L));

    // Transform fragPos to screen space
    vec4 fragPosNDC = mvp * vec4(fragPos, 1.0);   
    fragPosNDC.xyz /= fragPosNDC.w;               
    vec2 fragScreenCoord = fragPosNDC.xy * 0.5 + 0.5; // Map to [0,1] range

    vec3 color = vec3(1.0); 
    vec2 minCoord = overlayCenter - vec2(overlaySize / 2.0);
    vec2 maxCoord = overlayCenter + vec2(overlaySize / 2.0);

    if (fragScreenCoord.x >= minCoord.x && fragScreenCoord.x <= maxCoord.x &&
        fragScreenCoord.y >= minCoord.y && fragScreenCoord.y <= maxCoord.y) {
        vec2 overlayCoord = (fragScreenCoord - minCoord) / overlaySize;
        // Map second pass
        color = texture(texMinimap, overlayCoord).rgb;
    }

    outColor = vec4(color * diffuse, 1.0);
}
