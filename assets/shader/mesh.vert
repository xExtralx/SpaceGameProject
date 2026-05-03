#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 uViewProj;
uniform mat4 uModel;

out vec2 vUV;

void main() {
    gl_Position = vec4(aPos * 0.5, 1.0); // ignore tout, force NDC direct
    vUV = aUV;
}