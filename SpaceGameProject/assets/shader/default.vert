#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

uniform mat4 uProjection;
uniform float uTime;
uniform float uScale;

out vec4 vColor;

void main() {
    gl_Position = vec4(aPos.xy / 200.0, 0.0, 1.0);
    vColor = aColor;
}
