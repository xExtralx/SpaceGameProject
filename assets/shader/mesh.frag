#version 430 core

in vec2 vUV;

uniform sampler2D uTexture;
uniform int uIsOutline = 0;
uniform vec4 uOutlineColor = vec4(0.0, 0.0, 0.0, 1.0);

out vec4 FragColor;

void main() {
    if (uIsOutline == 1) {
        FragColor = uOutlineColor;
        return;
    }
    vec4 color = texture(uTexture, vUV);
    if (color.a < 0.01) discard;
    FragColor = color;
}