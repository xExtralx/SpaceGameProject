#version 330 core

in vec4 vColor;
in float vDepth;

out vec4 FragColor;

void main() {
    FragColor = vec4(vColor);
}
