#version 410

// Per-vertex attributes
in vec2 texPos;
in vec2 texCoords;

// Data to pass to fragment shader
out vec2 fragPos;
out vec2 fragCoords;
out vec4 particleColor;

uniform mat4 mvp;
uniform vec2 offset;

void main() {
    fragPos = texPos;
    fragCoords = texCoords;
    particleColor = vec4(1.0, 0.0, 0.0, 1.0); 
    float scale = 1.0;
    gl_Position = mvp * vec4(texPos.x * scale + offset.x, 0, texPos.y * scale + offset.y, 1.0); 
    // gl_PointSize = 5.0; 
}
