#version 330 core

in vec2  vUV;
in float vAO;
in float vFog;
in vec3  vTilePos;
in vec2 vUVOffset;
in vec2 vUVSize;

out vec4 FragColor;

uniform sampler2D uTileset;
uniform vec3      uAmbientColor; // ambient light color
uniform float     uAmbientStr;  // ambient strength
uniform float     uTime;        // for animated tiles

// Hash function for pseudo-random values from tile position
vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

vec4 sampleTile(sampler2D tex, vec2 uv, vec2 uvOffset, vec2 uvSize, vec3 tilePos) {
    // Random offset per tile based on its grid position
    vec2 rnd = hash2(floor(tilePos.xy));

    // Random UV offset (shift within the tile region only)
    vec2 shift = (rnd - 0.5) * 0.15; // 0.15 = max shift amount

    // Random rotation (small angle only to avoid obvious rotation)
    float angle = (rnd.x - 0.5) * 0.3; // ±0.15 radians
    float c = cos(angle);
    float s = sin(angle);
    mat2 rot = mat2(c, -s, s, c);

    // Center UV, rotate, uncenter
    vec2 centeredUV = uv - vec2(0.5);
    vec2 rotatedUV  = rot * centeredUV + vec2(0.5);

    // Apply shift and clamp within tile bounds
    vec2 finalUV = uvOffset + clamp(rotatedUV + shift, vec2(0.0), vec2(1.0)) * uvSize;

    return texture(tex, finalUV);
}

void main() {
    vec4 texColor = sampleTile(uTileset, 
        (vUV - iUVOffset) / iUVSize, // local UV 0-1
        iUVOffset, iUVSize, vTilePos);

    if (texColor.a < 0.1) discard;

    vec3 color = texColor.rgb * (1.0 - vAO * 0.4);
    color *= uAmbientColor * uAmbientStr;
    color = mix(color, color * 0.85, vFog);

    FragColor = vec4(color, texColor.a);
}