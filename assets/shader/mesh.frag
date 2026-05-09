// mesh.frag
#version 430 core

in vec2 vUV;
out vec4 FragColor;

void main() {
    FragColor = vec4(vUV.x, vUV.y, 0.0, 1.0);
}