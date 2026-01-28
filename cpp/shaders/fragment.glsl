#version 330 core

in vec2 vTexCoord;
in float vLight;
in float vFog;

uniform sampler2D uTexture;

out vec4 FragColor;

const vec3 FOG_COLOR = vec3(0.6, 0.8, 1.0);

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    if (texColor.a < 0.1)
        discard;

    vec3 color = texColor.rgb * vLight;
    color = mix(color, FOG_COLOR, vFog);
    FragColor = vec4(color, texColor.a);
}
