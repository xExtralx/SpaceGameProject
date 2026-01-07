#version 430 core
in vec4 vColor;
out vec4 FragColor;

in float uTime;

void main() {
    FragColor = (vColor.xyz * sin(uTime),255);
}
