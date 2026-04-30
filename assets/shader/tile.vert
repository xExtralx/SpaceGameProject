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
// out vec2  vUVOffset;
// out vec2  vUVSize;

uniform mat4  uViewProj;
uniform vec2  uTileSize;
uniform float uHeightStep;

vec2 isoProject(vec2 p) {
    float x = (p.x - p.y) * uTileSize.x * 0.5;
    float y = (p.x + p.y) * uTileSize.y * 0.5;
    return vec2(x, y);
}

void main() {
    vec2 iso = isoProject(iTilePos.xy);
    iso.y += 0.0; // disable height temporarily

    vec2 worldPos = iso + aLocalPos * uTileSize;

    float depth = 0.0; // disable depth sorting temporarily

    gl_Position = uViewProj * vec4(worldPos, depth, 1.0);

    vUV      = iUVOffset + aUV * iUVSize;
    vAO      = iAO;
    vFog     = 0.0;
    vTilePos = iTilePos;
    // vUVOffset = iUVOffset;
    // vUVSize   = iUVSize;
}