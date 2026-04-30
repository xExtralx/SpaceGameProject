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

// =====================
// HASH FUNCTIONS
// =====================
float hash1(vec2 p) {
    p = fract(p * vec2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

vec3 hash3(vec2 p) {
    vec3 q = vec3(dot(p, vec2(127.1, 311.7)),
                  dot(p, vec2(269.5, 183.3)),
                  dot(p, vec2(419.2, 371.9)));
    return fract(sin(q) * 43758.5453);
}

// =====================
// ROTATE UV AROUND CENTER
// =====================
vec2 rotateUV(vec2 uv, float angle) {
    float c   = cos(angle);
    float s   = sin(angle);
    vec2  centered = uv - 0.5;
    return vec2(c * centered.x - s * centered.y,
                s * centered.x + c * centered.y) + 0.5;
}

// =====================
// SAMPLE WITH VARIATION
// =====================
vec4 sampleVaried(vec2 localUV, vec2 uvOffset, vec2 uvSize, vec2 tileID) {
    vec2 rnd = hash2(tileID);

    // Flip horizontally or vertically
    if (rnd.x > 0.5) localUV.x = 1.0 - localUV.x;
    if (rnd.y > 0.5) localUV.y = 1.0 - localUV.y;

    // Rotate by 0, 90, 180 or 270 degrees only (no artifacts)
    int rotStep = int(rnd.x * 4.0);
    if (rotStep == 1) localUV = vec2(1.0 - localUV.y, localUV.x);
    if (rotStep == 2) localUV = vec2(1.0 - localUV.x, 1.0 - localUV.y);
    if (rotStep == 3) localUV = vec2(localUV.y, 1.0 - localUV.x);

    // Clamp to avoid bleeding into neighboring tiles
    localUV = clamp(localUV, 0.001, 0.999);

    return texture(uTileset, uvOffset + localUV * uvSize);
}

void main() {
    // Tile grid position (integer)
    vec2 tileID  = floor(vTilePos.xy);

    // Local UV within this tile (0-1)
    // vUV is already mapped to atlas, recover local UV
    vec2 rnd     = hash2(tileID);
    vec3 rnd3    = hash3(tileID);
    float h1     = hash1(tileID);

    // Sample base texture
    vec4 texColor = texture(uTileset, vUV);
    if (texColor.a < 0.1) discard;

    vec3 color = texColor.rgb;

    // =====================
    // 1. BRIGHTNESS VARIATION
    // per tile random brightness ±15%
    // =====================
    float brightness = 0.85 + rnd.x * 0.30;
    color *= brightness;

    // =====================
    // 2. COLOR TINT VARIATION
    // subtle warm/cool shift per tile
    // =====================
    vec3 warmTint = vec3(1.05, 1.0,  0.95);
    vec3 coolTint = vec3(0.95, 1.0,  1.05);
    vec3 tint     = mix(coolTint, warmTint, rnd.y);
    color        *= tint;

    // =====================
    // 3. MICRO CONTRAST
    // slightly boost or reduce contrast per tile
    // =====================
    float contrast = 0.9 + h1 * 0.2;
    color = (color - 0.5) * contrast + 0.5;
    color = clamp(color, 0.0, 1.0);

    // =====================
    // 4. SUBTLE NOISE OVERLAY
    // adds micro detail to break flat areas
    // =====================
    float noiseVal = fract(sin(dot(vUV * 100.0, vec2(12.9898, 78.233))) * 43758.5453);
    color += (noiseVal - 0.5) * 0.03; // very subtle ±1.5%

    // =====================
    // 5. VIGNETTE PER TILE
    // darkens tile edges slightly for separation
    // =====================
    // Recover local UV from world position
    vec2 localUV = fract(vTilePos.xy);
    // Convert to -1..1
    vec2 vigUV   = localUV * 2.0 - 1.0;
    // Smooth vignette
    float vignette = 1.0 - dot(vigUV * 0.4, vigUV * 0.4);
    vignette = clamp(vignette, 0.7, 1.0);
    color *= vignette;

    // =====================
    // 6. AMBIENT OCCLUSION
    // =====================
    color *= (1.0 - vAO * 0.4);

    // =====================
    // 7. AMBIENT LIGHT
    // =====================
    color *= uAmbientColor * uAmbientStr;

    // =====================
    // 8. HEIGHT FOG
    // =====================
    color = mix(color, color * 0.85, clamp(vFog, 0.0, 1.0));

    FragColor = vec4(color, texColor.a);
}