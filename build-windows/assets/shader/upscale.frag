#version 430 core

in vec2 vUV;
uniform sampler2D uTexture;
uniform sampler2D uDepth;

out vec4 FragColor;

void main() {
    vec2 texel = 1.0 / vec2(320.0, 180.0);

    // Sample depth des 4 voisins
    float d  = texture(uDepth, vUV).r;
    float dR = texture(uDepth, vUV + vec2(texel.x, 0.0)).r;
    float dU = texture(uDepth, vUV + vec2(0.0, texel.y)).r;
    float dL = texture(uDepth, vUV - vec2(texel.x, 0.0)).r;
    float dD = texture(uDepth, vUV - vec2(0.0, texel.y)).r;

    // Différence de profondeur = arête
    float edge = abs(d - dR) + abs(d - dU) + abs(d - dL) + abs(d - dD);
    edge = step(0.0001, edge); // seuil : 1 = arête, 0 = plat

    vec4 color = texture(uTexture, vUV);
    FragColor = mix(color, vec4(0.0, 0.0, 0.0, 1.0), edge); // arêtes noires
}