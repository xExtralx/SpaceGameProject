#version 330 core

layout (location = 0) in vec2 aLocalPos;  // quad corner (-0.5 to 0.5)
layout (location = 1) in vec2 aUV;        // quad UV

// Per instance
layout (location = 2) in vec3  iTilePos;  // tile grid x, y, height
layout (location = 3) in vec2  iUVOffset; // tileset UV offset
layout (location = 4) in float iAO;       // ambient occlusion 0-1

out vec2  vUV;
out float vAO;
out float vFog;
out vec3  vTilePos;

uniform mat4  uViewProj;
uniform vec2  uTileSize;    // isometric tile width, height in pixels
uniform float uHeightStep;  // pixels per height unit

vec2 isoProject(vec2 p) {
    float x = (p.x - p.y) * uTileSize.x * 0.5;
    float y = (p.x + p.y) * uTileSize.y * 0.5;
    return vec2(x, y);
}

void main() {
    // Isometric projection of tile grid position
    vec2 iso = isoProject(iTilePos.xy);

    // Visual height offset
    iso.y += iTilePos.z * uHeightStep;

    // Final world position of this quad corner
    vec2 worldPos = iso + aLocalPos * uTileSize;

    // Stable isometric depth sorting
    float depth = (iTilePos.x + iTilePos.y) * 0.001
                + iTilePos.z * 0.0001;

    gl_Position = uViewProj * vec4(worldPos, depth, 1.0);

    vUV      = iUVOffset + aUV;
    vAO      = iAO;
    vFog     = iTilePos.z * 0.05; // slight height fog
    vTilePos = iTilePos;
}