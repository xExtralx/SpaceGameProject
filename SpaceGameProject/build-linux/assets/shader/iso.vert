#version 330 core

layout(location = 0) in float aZ;
layout(location = 1) in vec2  aUV;
layout(location = 2) in vec2  aPos;
layout(location = 3) in vec4  aColor;

out vec2 vUV;
out vec4 vColor;

uniform mat4 uViewProj;
uniform vec2 uTileSize;
uniform vec2 uChunkOrigin;

vec2 isoProject(vec2 p)
{
    float isoX = (p.x - p.y) * (uTileSize.x * 0.5);
    float isoY = (p.x + p.y) * (uTileSize.y * 0.5);
    return vec2(isoX, isoY);
}

void main()
{
    vec2 gridPos = aPos + uChunkOrigin;
    vec2 iso = isoProject(gridPos);

    // Z-order iso :
    // plus (x + y) est grand, plus c'est "loin"
    float depth = aZ + gridPos.x + gridPos.y;

    gl_Position = uViewProj * vec4(iso, depth * 0.0001, 1.0);

    vUV = aUV;
    vColor = aColor;
}
