#version 460

precision mediump float;

uniform vec2 resolution;
uniform int seed_radius;
uniform vec4 seed_color;

in vec4 color;
in vec2 seed;
out vec4 out_color;

void main(void) {
    float dx = gl_FragCoord.x - seed.x;
    float dy = gl_FragCoord.y - seed.y;
    float rx = resolution.x;
    float ry = resolution.y;

    int c = int(length(gl_FragCoord.xy - seed) >= seed_radius);
    gl_FragDepth = c * pow(abs(dx*dx*dx) + abs (dy*dy*dy), 1.0/3.0) / pow(abs(rx*rx*rx) + abs(ry*ry*ry), 1.0/3.0);
    out_color = c * color + (1 - c) * seed_color;
}