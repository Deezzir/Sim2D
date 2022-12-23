#version 460

precision mediump float;

uniform vec2 resolution;
uniform int radius;
uniform vec4 seed_color;

in vec4 color;
in vec2 seed;
out vec4 out_color;

void main(void) {
    int c = int(length(gl_FragCoord.xy - seed) >= radius);
    gl_FragDepth = c *  length(gl_FragCoord.xy - seed)/length(resolution);
    out_color = c * color + (1 - c) * seed_color;
}