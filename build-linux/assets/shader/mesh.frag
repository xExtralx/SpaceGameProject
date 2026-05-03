#version 430 core

in vec2 vUV;

uniform sampler2D uTexture;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(uTexture, vUV);
    if (texColor.a < 0.1) discard;

    // Lumière ambiante simple pour voir le mesh
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // rouge fixe
}