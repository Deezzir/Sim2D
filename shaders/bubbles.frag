#version 460

precision mediump float;

uniform vec2 resolution;
uniform vec4 seed_mark_color;

in vec2 seed_pos;
flat in int seed_mark_rad;

out vec4 out_color;

void main(void) {
    float r = seed_mark_rad / length(resolution);
    float d = length(gl_FragCoord.xy - seed_pos) / length(resolution);
    
    if (d < r) {
        gl_FragDepth = d / r;
        out_color = mix(vec4(0, 0, 0, 1), vec4(1, 1, 1, 1), d / r / 1.2);
    } else {
        gl_FragDepth = 1;
        out_color = vec4(0, 0, 0, 0);
    }
} 