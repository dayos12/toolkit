#version 300 es
precision highp float;
layout(location = 0) in vec4 aPosition;
out vec2 vTexCoord;

void main() {
    gl_Position = aPosition;
    vTexCoord = aPosition.xy * 0.5 + 0.5;
}
