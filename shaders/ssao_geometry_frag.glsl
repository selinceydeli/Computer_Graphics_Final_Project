#version 410

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 fragPos;
in vec3 fragNormal;

void main()
{    
    gPosition = vec4(fragPos, 1.0);       
    gNormal = normalize(fragNormal);         
    gAlbedoSpec.rgb = vec3(0.95);          
}
