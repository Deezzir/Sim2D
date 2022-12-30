#version 460

precision mediump float;

uniform vec2 resolution;
uniform vec4 seed_mark_color;

in vec2 seed_pos;
in vec4 seed_color;
flat in int seed_mark_rad;

out vec4 out_color;

void main(void) {
    if (length(gl_FragCoord.xy - seed_pos) < seed_mark_rad) {
        gl_FragDepth = 0;
        out_color = seed_mark_color;
    } else {
        float d = length(gl_FragCoord.xy - seed_pos) / length(resolution);
        float r = seed_mark_rad * 10 / length(resolution);
        if (d < r) {
            gl_FragDepth = d;
            out_color = seed_color;
        } else {
            gl_FragDepth = 1;
            out_color = seed_mark_color;
        }
    }
} 