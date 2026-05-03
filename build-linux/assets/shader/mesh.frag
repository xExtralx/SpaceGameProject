#version 430 core

in vec2 vUV;

uniform sampler2D uTexture;

// Simple directional light pour donner du volume
uniform vec3  uLightDir    = normalize(vec3(1.0, 2.0, 1.0));
uniform vec3  uLightColor  = vec3(1.0, 0.95, 0.8);
uniform float uAmbient     = 0.4;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(uTexture, vUV);
    if (texColor.a < 0.1) discard;

    FragColor = texColor;
}