#ifndef _HELPERS_H
#define _HELPERS_H

typedef struct {
    float x, y;
} vec2;

typedef struct {
    float x, y, z, w;
} vec4;

// Function declarations
// ---------------------
float rand_float();
float lerpf(float start, float end, float t);

float sqr_dist(float x1, float y1, float x2, float y2);
float dist(float x1, float y1, float x2, float y2);
float dot(float x1, float y1, float x2, float y2);

float sqr_dist_vec2(vec2 v1, vec2 v2);
float dist_vec2(vec2 v1, vec2 v2);
float sqr_mag_vec2(vec2 v);
float mag_vec2(vec2 v);
float dot_vec2(vec2 v1, vec2 v2);
vec2 vec2_mul(vec2 v1, vec2 v2);
vec2 vec2_add(vec2 v1, vec2 v2);
vec2 vec2_sub(vec2 v1, vec2 v2);
vec2 vec2_mul_scalar(vec2 v, float s);
vec2 vec2_div_scalar(vec2 v, float s);
vec2 vec2_normalize(vec2 v);

vec2 collision_sim(vec2 pos1, vec2 pos2, vec2 vel1, vec2 vel2, float r1, float r2);

#endif // HELPERS_H