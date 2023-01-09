#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdbool.h>

typedef struct {
    float x, y;
} vec2;

typedef struct {
    float x, y, z, w;
} vec4;

// Function declarations
// ---------------------
void usage();
void get_arguments(int argc, char **argv);
void init_signal_handler();

float rand_float();
float lerpf(float start, float end, float t);

float vec2_sqr_dist(vec2 v1, vec2 v2);
float vec2_dist(vec2 v1, vec2 v2);
float vec2_sqr_mag(vec2 v);
float vec2_mag(vec2 v);
float vec2_dot(vec2 v1, vec2 v2);

vec2 vec2_mul(vec2 v1, vec2 v2);
vec2 vec2_add(vec2 v1, vec2 v2);
vec2 vec2_sub(vec2 v1, vec2 v2);
vec2 vec2_mul_scalar(vec2 v, float s);
vec2 vec2_div_scalar(vec2 v, float s);
vec2 vec2_normalize(vec2 v);
vec2 vec2_unit_normal(vec2 v1, vec2 v2);
vec2 vec2_unit_tangent(vec2 v1, vec2 v2);

bool vec2_is_zero(vec2 v);

void collision_sim_0(vec2 pos1, vec2 pos2, vec2 vel1, vec2 vel2, float r1, float r2, vec2* vel1_new, vec2* vel2_new);
void collision_sim_1(vec2 pos1, vec2 pos2, vec2 vel1, vec2 vel2, float m1, float m2, vec2* vel1_new, vec2* vel2_new);

#endif  // HELPERS_H