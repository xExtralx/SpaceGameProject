#version 430 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aUV;

uniform mat4 uViewProj;
uniform mat4 uModel;

void main() {
    gl_Position = uViewProj * uModel * vec4(aPos, 1.0);
}