#version 330 core

in vec2  vUV;
in float vAO;
in float vFog;
in vec3  vTilePos;

out vec4 FragColor;

uniform sampler2D uTileset;
uniform vec3      uAmbientColor; // ambient light color
uniform float     uAmbientStr;  // ambient strength
uniform float     uTime;        // for animated tiles

void main() {
    vec4 texColor = texture(uTileset, vUV);
    if (texColor.a < 0.1) discard;

    // Ambient occlusion darkens edges
    vec3 color = texColor.rgb * (1.0 - vAO * 0.4);

    // Ambient light tint
    color *= uAmbientColor * uAmbientStr;

    // Height fog — slightly darker at ground level
    color = mix(color, color * 0.85, vFog);

    FragColor = vec4(color, texColor.a);
}