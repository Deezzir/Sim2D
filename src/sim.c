#include <assert.h>
#include <math.h>

#include "main.h"

vec2 seed_positions[SEED_COUNT];
vec2 seed_velocities[SEED_COUNT];
vec4 seed_colors[SEED_COUNT];
GLint seed_mark_radii[SEED_COUNT];

void generate_voronoi_seeds() {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        seed_positions[i].x = rand_float() * DEFAULT_SCREEN_WIDTH;
        seed_positions[i].y = rand_float() * DEFAULT_SCREEN_HEIGHT;

        seed_colors[i].x = rand_float();
        seed_colors[i].y = rand_float();
        seed_colors[i].z = rand_float();
        seed_colors[i].w = 1.0f;

        seed_mark_radii[i] = SEED_MARK_RADIUS;

        float angle = rand_float() * 2.0f * M_PI;
        float mag = lerpf(100, 200, rand_float());
        seed_velocities[i].x = cosf(angle) * mag;
        seed_velocities[i].y = sinf(angle) * mag;
    }
}

void generate_bubbles_seeds() {
    assert(0 && "Not implemented yet");
}

void render_voronoi_frame(double delta_time, int width, int height) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (size_t i = 0; i < SEED_COUNT; i++) {
        vec2 pos1 = vec2_add(seed_positions[i], vec2_mul_scalar(seed_velocities[i], delta_time));

        // Bounce off other seeds
        for (size_t j = i + 1; j < SEED_COUNT; j++) {
            vec2 pos2 = seed_positions[j];
            float dist = dist_vec2(pos1, pos2);

            if (dist < (seed_mark_radii[i] + seed_mark_radii[j])) {
                vec2 vel1 = seed_velocities[i];
                vec2 vel2 = seed_velocities[j];

                seed_velocities[i] = collision_sim(pos1, pos2, vel1, vel2, seed_mark_radii[i], seed_mark_radii[j]);
                seed_velocities[j] = collision_sim(pos2, pos1, vel2, vel1, seed_mark_radii[j], seed_mark_radii[i]);
            }
        }

        // Bounce off walls
        if ((pos1.x < 0.0f && seed_velocities[i].x < 0) || (pos1.x > width && seed_velocities[i].x > 0)) {
            seed_velocities[i] = vec2_mul(seed_velocities[i], (vec2){-1.0f, 1.0f});
        }
        if ((pos1.y < 0.0f && seed_velocities[i].y < 0) || (pos1.y > height && seed_velocities[i].y > 0)) {
            seed_velocities[i] = vec2_mul(seed_velocities[i], (vec2){1.0f, -1.0f});
        }

        seed_positions[i] = pos1;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbos[ATTRIB_POS]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(seed_positions), seed_positions);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, SEED_COUNT);
}

void render_bubbles_frame(double delta_time, int width, int height) {
    (void)delta_time;
    (void)width;
    (void)height;
    assert(0 && "Not implemented yet");
}