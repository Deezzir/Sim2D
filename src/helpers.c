#include "helpers.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

float rand_float() {
    return (float)rand() / (float)RAND_MAX;
}

float lerpf(float start, float end, float t) {
    return start + (end - start) * t;
}

float sqr_dist(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return dx * dx + dy * dy;
}

float dist(float x1, float y1, float x2, float y2) {
    return sqrtf(sqr_dist(x1, y1, x2, y2));
}

float dot(float x1, float y1, float x2, float y2) {
    return x1 * x2 + y1 * y2;
}

float sqr_dist_vec2(vec2 v1, vec2 v2) {
    return sqr_dist(v1.x, v1.y, v2.x, v2.y);
}

float dist_vec2(vec2 v1, vec2 v2) {
    return dist(v1.x, v1.y, v2.x, v2.y);
}

float sqr_mag_vec2(vec2 v) {
    return sqr_dist(v.x, v.y, 0.0f, 0.0f);
}

float mag_vec2(vec2 v) {
    return dist(v.x, v.y, 0.0f, 0.0f);
}

float dot_vec2(vec2 v1, vec2 v2) {
    return dot(v1.x, v1.y, v2.x, v2.y);
}

vec2 vec2_mul(vec2 v1, vec2 v2) {
    return (vec2){v1.x * v2.x, v1.y * v2.y};
}

vec2 vec2_add(vec2 v1, vec2 v2) {
    return (vec2){v1.x + v2.x, v1.y + v2.y};
}

vec2 vec2_sub(vec2 v1, vec2 v2) {
    return (vec2){v1.x - v2.x, v1.y - v2.y};
}

vec2 vec2_mul_scalar(vec2 v, float s) {
    return (vec2){v.x * s, v.y * s};
}

vec2 vec2_div_scalar(vec2 v, float s) {
    assert(s != 0.0f);
    return (vec2){v.x / s, v.y / s};
}

vec2 vec2_normalize(vec2 v) {
    float mag = mag_vec2(v);
    assert(mag != 0.0f);

    return (vec2){v.x / mag, v.y / mag};
}

vec2 collision_sim(vec2 pos1, vec2 pos2, vec2 vel1, vec2 vel2, float m1, float m2) {
    assert(m1 > 0.0f && m2 > 0.0f);
    assert(sqr_dist_vec2(pos1, pos2) > 0.0f);

    // new_vel = vel1 - (2 * m1 / (m1 + m2)) * ((vel1 - vel2) • (pos1 - pos2)) / ||pos1 - pos2||^2 * (pos1 - pos2)
    vec2 pos_diff = vec2_sub(pos1, pos2);
    vec2 vel_diff = vec2_sub(vel1, vel2);

    float new_vel_mag = (2.0f * m1) / (m1 + m2) * dot_vec2(vel_diff, pos_diff) / sqr_mag_vec2(pos_diff);

    vec2 new_vel = vec2_sub(vel1, vec2_mul_scalar(pos_diff, new_vel_mag));

    return new_vel;
}