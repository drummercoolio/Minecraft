#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in float aLight;

uniform mat4 uProjection;
uniform mat4 uView;

out vec2 vTexCoord;
out float vLight;
out float vFog;

const float FOG_START = 120.0;
const float FOG_END = 160.0;

void main() {
    vec4 viewPos = uView * vec4(aPos, 1.0);
    gl_Position = uProjection * viewPos;

    vTexCoord = aTexCoord;
    vLight = aLight;

    float dist = length(viewPos.xyz);
    vFog = clamp((dist - FOG_START) / (FOG_END - FOG_START), 0.0, 1.0);
}
