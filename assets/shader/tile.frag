#version 330 core

in vec2  vUV;
in float vAO;
in float vFog;
in vec3  vTilePos;

out vec4 FragColor;

uniform sampler2D uTileset;
uniform vec3      uAmbientColor;
uniform float     uAmbientStr;
uniform float     uTime;

// Simple hash from tile position
vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

void main() {
    vec4 texColor = texture(uTileset, vUV);
    if (texColor.a < 0.1) discard;

    // Random value per tile
    vec2 rnd = hash2(floor(vTilePos.xy));

    // Brightness variation ±12%
    float brightness = 0.88 + rnd.x * 0.24;

    // Subtle color shift
    vec3 colorShift = vec3(
        0.97 + rnd.y * 0.06,
        1.00,
        0.97 + rnd.x * 0.06
    );

    vec3 color = texColor.rgb * brightness * colorShift;

    // Ambient
    color *= uAmbientColor * uAmbientStr;

    // Height fog
    color = mix(color, color * 0.85, vFog);

    // AO
    color *= (1.0 - vAO * 0.4);

    FragColor = vec4(color, texColor.a);
}