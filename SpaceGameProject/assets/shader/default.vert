#version 330 core

layout (location = 0) in vec2 aLocalPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aTilePos; // x, y, height

out vec2 vUV;

uniform mat4 uViewProj;
uniform vec2 uTileSize;
uniform float uHeightStep;

vec2 isoProject(vec2 p)
{
    float x = (p.x - p.y) * uTileSize.x * 0.5;
    float y = (p.x + p.y) * uTileSize.y * 0.5;
    return vec2(x, y);
}

void main()
{
    // position iso de la tuile
    vec2 iso = isoProject(aTilePos.xy);

    // hauteur visuelle
    iso.y += aTilePos.z * uHeightStep;

    // position finale du coin du quad
    vec2 worldPos = iso + aLocalPos * uTileSize;

    // ordre de dessin iso simple et stable
    float depth = (aTilePos.x + aTilePos.y) * 0.01 + aTilePos.z * 0.001;

    gl_Position = uViewProj * vec4(worldPos, depth, 1.0);
    vUV = aUV;
}
