#version 460

precision mediump float;

#define SEED_MARK_COLOR vec4(0, 0, 0, 1)

uniform vec2 resolution;

in vec2 seed_pos;
in vec4 seed_color;
flat in int seed_mark_rad;

out vec4 out_color;

void main(void) {
    float dx = gl_FragCoord.x - seed_pos.x;
    float dy = gl_FragCoord.y - seed_pos.y;
    float rx = resolution.x;
    float ry = resolution.y;

    int c = int(length(gl_FragCoord.xy - seed_pos) >= seed_mark_rad);
    gl_FragDepth = c * pow(abs(dx*dx*dx) + abs (dy*dy*dy), 1.0/3.0) / pow(abs(rx*rx*rx) + abs(ry*ry*ry), 1.0/3.0);
    out_color = c * seed_color + (1 - c) * SEED_MARK_COLOR;
}