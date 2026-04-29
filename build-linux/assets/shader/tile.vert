#version 330 core

layout (location = 0) in vec2  aLocalPos;
layout (location = 1) in vec2  aUV;

// Per instance
layout (location = 2) in vec3  iTilePos;
layout (location = 3) in vec2  iUVOffset;
layout (location = 4) in vec2  iUVSize;   // added
layout (location = 5) in float iAO;       // bumped from 4 to 5

out vec2  vUV;
out float vAO;
out float vFog;
out vec3  vTilePos;

uniform mat4  uViewProj;
uniform vec2  uTileSize;
uniform float uHeightStep;

vec2 isoProject(vec2 p) {
    // Each tile step moves 128px right/left and 64px up/down
    float x = (p.x - p.y) * 128.0f;
    float y = (p.x + p.y) * 64.0f;
    return vec2(x, y);
}

void main() {
    vec2 iso = isoProject(iTilePos.xy);
    iso.y += iTilePos.z * uHeightStep;

    // aLocalPos is already in pixels, no need to multiply by uTileSize
    vec2 worldPos = iso + aLocalPos;

    float depth = (iTilePos.x + iTilePos.y) * 0.001
                + iTilePos.z * 0.0001;

    gl_Position = uViewProj * vec4(worldPos, depth, 1.0);

    vUV      = iUVOffset + aUV * iUVSize;
    vAO      = iAO;
    vFog     = iTilePos.z * 0.05;
    vTilePos = iTilePos;
}