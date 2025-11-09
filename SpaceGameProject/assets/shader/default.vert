#version 330 core

layout (location = 0) in vec3 aPos;     // position en pixels
layout (location = 1) in vec4 aColor;   // couleur

out vec4 vColor;
out float vDepth;

uniform vec2 uResolution;

void main()
{
    // Convertir les coordonnées pixel -> Normalized Device Coordinates ([-1,+1])
    vec2 pos = aPos.xy / uResolution * 2.0 - 1.0;

    gl_Position = vec4(pos,aPos.z, 1.0);
    vColor = aColor;
    vDepth = aPos.z;
}
