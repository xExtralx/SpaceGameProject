#version 330 core

in vec2 vUV;
in vec4 vColor;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform bool uUseTexture;

void main()
{
    vec4 texColor = uUseTexture ? texture(uTexture, vUV) : vec4(1.0);
    FragColor = texColor * vColor;
}
