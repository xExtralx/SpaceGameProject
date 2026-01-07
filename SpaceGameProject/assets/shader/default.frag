#version 430 core

in vec4 vColor;
out vec4 FragColor;

uniform float uTime;

void main() {
    FragColor = vec4(vColor.xyz, 1.0);
}
