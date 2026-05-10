uniform int uIsOutline;
uniform vec4 uOutlineColor;

void main() {
    if (uIsOutline == 1) {
        FragColor = uOutlineColor;
        return;
    }
    vec4 color = texture(uTexture, vUV);
    if (color.a < 0.01) discard;
    FragColor = color;
}