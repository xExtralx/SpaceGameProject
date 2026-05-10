#version 430 core
in vec2 vUV;
uniform sampler2D uTexture;
uniform sampler2D uMask;

out vec4 FragColor;

void main() {
    vec2 texel = 1.0 / vec2(320.0, 180.0);

    vec3 id  = texture(uMask, vUV).rgb;
    vec3 idR = texture(uMask, vUV + vec2(texel.x, 0.0)).rgb;
    vec3 idU = texture(uMask, vUV + vec2(0.0, texel.y)).rgb;
    vec3 idL = texture(uMask, vUV - vec2(texel.x, 0.0)).rgb;
    vec3 idD = texture(uMask, vUV - vec2(0.0, texel.y)).rgb;

    float edge = 0.0;
    edge += float(id != idR);
    edge += float(id != idU);
    edge += float(id != idL);
    edge += float(id != idD);
    edge = min(edge, 1.0);

    vec4 color = texture(uTexture, vUV);
    FragColor = mix(color, vec4(1.0, 0.0, 0.0, 1.0), edge);
}