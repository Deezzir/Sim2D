#version 460

precision mediump float;

uniform vec2 resolution;
uniform vec4 seed_color;

in vec4 color;
in vec2 seed;
in int radius;
out vec4 out_color;

void main(void) {
    if (length(gl_FragCoord.xy - seed) < radius) {
        gl_FragDepth = 0;
        out_color = seed_color;
    } else {
        float d = length(gl_FragCoord.xy - seed) / length(resolution);
        if (d < radius) {
            gl_FragDepth = d;
            out_color = color;
        } else {
            gl_FragDepth = 1;
            out_color = seed_color;
        }
    }
} 