#version 430 core
in vec2 vUV;
uniform sampler2D uTexture;
uniform sampler2D uMask;

out vec4 FragColor;

void main() {
    vec4 maskColor = texture(uMask, vUV);
    FragColor = maskColor; // affiche le mask brut
}