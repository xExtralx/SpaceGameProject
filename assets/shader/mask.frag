#version 430 core
uniform int uObjectID; // passé par renderMesh
out vec4 FragColor;

void main() {
    // Encode ID en RGB normalisé
    float r = float((uObjectID)       & 0xFF) / 255.0;
    float g = float((uObjectID >> 8)  & 0xFF) / 255.0;
    float b = float((uObjectID >> 16) & 0xFF) / 255.0;
    FragColor = vec4(r, g, b, 1.0);
}