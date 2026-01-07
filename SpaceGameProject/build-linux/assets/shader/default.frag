#version 430 core

in vec4 vColor;
out vec4 FragColor;

uniform float uTime;

void main() {
    float s = sin(uTime);
    FragColor = vec4(vColor.xyz * s, 1.0);
}
