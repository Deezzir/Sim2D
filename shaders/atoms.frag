#version 460

precision mediump float;

#define BACKGROUND_COLOR vec4(0.1, 0.1, 0.1, 1)
#define SEED_COLOR vec4(0.9, 0.9, 0.9, 1)

uniform vec2 resolution;

in vec2 seed_pos;
in vec4 seed_color;
flat in int seed_mark_rad;

out vec4 out_color;

void main(void) {
    if (length(gl_FragCoord.xy - seed_pos) < seed_mark_rad) {
        gl_FragDepth = 0;
        out_color = seed_color;
    } else {
        gl_FragDepth = length(gl_FragCoord.xy - seed_pos)/length(resolution);
        out_color = BACKGROUND_COLOR;
    }
}