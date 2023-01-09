#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"

#define GRAVITY ((vec2){0.0f, -9.8f})

// This source inner helpers
void _allocate_seed_memory();

void _generate_seed_pos(int seed_idx);
void _generate_seed_color(int seed_idx);
void _generate_seed_velocity(int seed_idx, float mag);

void _apply_constraints(int width, int height);
void _update_positions(double delta_time);
void _check_drag(GLFWwindow* window, int height, double delta_time);
void _solve_collisions_voronoi();
void _solve_collisions_bubbles();

void _generate_voronoi_seeds();
void _generate_bubbles_seeds();
void _render_voronoi_frame(GLFWwindow* window, double delta_time, int width, int height);
void _render_bubbles_frame(GLFWwindow* window, double delta_time, int width, int height);

static_assert(COUNT_MODES == 3, "Update list of mode names");
const char* mode_names[COUNT_MODES] = {
    [MODE_VORONOI] = "Voronoi",
    [MODE_ATOMS] = "Atoms",
    [MODE_BUBBLES] = "Bubbles",
};

int SEED_RADIUS = DEFAULT_SEED_RADIUS;
size_t SEED_COUNT = DEFAULT_SEED_COUNT;

vec2* seed_positions = NULL;
vec2* seed_velocities = NULL;
vec4* seed_colors = NULL;
GLint* seed_mark_radii = NULL;

int drag_seed_index = -1;
vec2 last_mouse_pos = {0.0f, 0.0f};
void (*_solve_collisions)(void) = NULL;

// Function definitions
// ---------------------
void init_sim_mode(Mode mode) {
    _allocate_seed_memory();

    switch (mode) {
        case MODE_VORONOI:
        case MODE_ATOMS:
            _generate_voronoi_seeds();
            _solve_collisions = _solve_collisions_voronoi;
            render_frame = _render_voronoi_frame;
            break;
        case MODE_BUBBLES:
            _generate_bubbles_seeds();
            _solve_collisions = _solve_collisions_bubbles;
            render_frame = _render_bubbles_frame;
            break;
        default:
            UNREACHABLE("Unexpected execution mode");
    }

    assert(_solve_collisions != NULL || "_solve_collisions is NULL");
    assert(render_frame != NULL || "render_frame is NULL");

    printf("Running '%s' mode\n", mode_names[mode]);
}

void free_sim_mode() {
    free(seed_positions);
    free(seed_velocities);
    free(seed_colors);
    free(seed_mark_radii);
}

// Private function definitions
// ---------------------
void _allocate_seed_memory() {
    if (seed_positions != NULL || seed_velocities != NULL || seed_colors != NULL || seed_mark_radii != NULL)
        free_sim_mode();

    seed_positions = (vec2*)calloc(SEED_COUNT, sizeof(vec2));
    seed_velocities = (vec2*)calloc(SEED_COUNT, sizeof(vec2));
    seed_colors = (vec4*)calloc(SEED_COUNT, sizeof(vec4));
    seed_mark_radii = (GLint*)calloc(SEED_COUNT, sizeof(GLint));

    if (seed_positions == NULL || seed_velocities == NULL || seed_colors == NULL || seed_mark_radii == NULL) {
        printf("[ERROR]: Memory was not allocated\n");
        exit(EXIT_FAILURE);
    }
}

void _generate_seed_pos(int seed_idx) {
    seed_positions[seed_idx].x = DEFAULT_SCREEN_WIDTH / 2 + rand_float() * 100 - 50;
    seed_positions[seed_idx].y = DEFAULT_SCREEN_HEIGHT / 2 + rand_float() * 100 - 50;
}

void _generate_seed_color(int seed_idx) {
    seed_colors[seed_idx].x = rand_float();
    seed_colors[seed_idx].y = rand_float();
    seed_colors[seed_idx].z = rand_float();
    seed_colors[seed_idx].w = 1.0f;
}

void _generate_seed_velocity(int seed_idx, float mag) {
    float angle = rand_float() * 2.0f * M_PI;
    seed_velocities[seed_idx].x = cosf(angle) * mag;
    seed_velocities[seed_idx].y = sinf(angle) * mag;
}

void _apply_constraints(int width, int height) {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        vec2 pos = seed_positions[i];

        // Bounce off walls
        if ((pos.x < 0.0f && seed_velocities[i].x < 0) || (pos.x > width && seed_velocities[i].x > 0)) {
            seed_velocities[i] = vec2_mul(seed_velocities[i], (vec2){-1.0f, 1.0f});
        }
        if ((pos.y < 0.0f && seed_velocities[i].y < 0) || (pos.y > height && seed_velocities[i].y > 0)) {
            seed_velocities[i] = vec2_mul(seed_velocities[i], (vec2){1.0f, -1.0f});
        }
    }
}

void _solve_collisions_voronoi() {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        vec2 pos1 = seed_positions[i];

        for (size_t j = i + 1; j < SEED_COUNT; j++) {
            vec2 pos2 = seed_positions[j];
            float dist = vec2_dist(pos1, pos2);
            float radii_sum = seed_mark_radii[i] + seed_mark_radii[j];

            if (dist < radii_sum) {
                vec2 vel1 = seed_velocities[i];
                vec2 vel2 = seed_velocities[j];
                int rad1 = seed_mark_radii[i];
                int rad2 = seed_mark_radii[j];

                if (vec2_is_zero(vel1)&& (int)i == drag_seed_index) {
                    rad1 = 1000000;
                } else if (vec2_is_zero(vel2) && (int)j == drag_seed_index) {
                    rad2 = 1000000;
                }

                collision_sim_0(pos1, pos2, vel1, vel2, rad1, rad2,
                                &seed_velocities[i], &seed_velocities[j]);

                // Move the current seed apart
                float delta = radii_sum - dist;
                vec2 n = vec2_div_scalar(vec2_sub(pos1, pos2), dist);
                seed_positions[i] = vec2_add(seed_positions[i], vec2_mul_scalar(n, delta * 0.5f));
                seed_positions[j] = vec2_add(seed_positions[j], vec2_mul_scalar(n, delta * -0.5f));
            }
        }
    }
}

void _solve_collisions_bubbles() {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        vec2 pos1 = seed_positions[i];

        for (size_t j = i + 1; j < SEED_COUNT; j++) {
            vec2 pos2 = seed_positions[j];
            float dist = vec2_dist(pos1, pos2);
            float radii_sum = seed_mark_radii[i] + seed_mark_radii[j];

            if (dist < radii_sum / 1.5f) {
                vec2 vel1 = seed_velocities[i];
                vec2 vel2 = seed_velocities[j];

                collision_sim_0(pos1, pos2, vel1, vel2,
                                seed_mark_radii[i], seed_mark_radii[j],
                                &seed_velocities[i], &seed_velocities[j]);

                // Move the current seed apart
                float delta = radii_sum / 1.5f - dist;
                vec2 n = vec2_div_scalar(vec2_sub(pos1, pos2), dist);
                seed_positions[i] = vec2_add(seed_positions[i], vec2_mul_scalar(n, delta * 0.5f));
                seed_positions[j] = vec2_add(seed_positions[j], vec2_mul_scalar(n, delta * -0.5f));
            }
        }
    }
}

void _update_positions(double delta_time) {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        if (drag_seed_index != (int)i)
            seed_positions[i] = vec2_add(seed_positions[i], vec2_mul_scalar(seed_velocities[i], delta_time));
    }
}

void _check_drag(GLFWwindow* window, int height, double delta_time) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    vec2 cur_mouse_pos = (vec2){(float)xpos, height - (float)ypos};

    if (IS_DRAG_MODE) {
        for (size_t i = 0; i < SEED_COUNT && drag_seed_index == -1; i++) {
            float dist = vec2_dist(seed_positions[i], cur_mouse_pos);
            if (dist < seed_mark_radii[i]) {
                drag_seed_index = i;
                break;
            }
        }

        if (drag_seed_index != -1) {
            seed_positions[drag_seed_index] = cur_mouse_pos;

            vec2 delta_cursor = vec2_sub(cur_mouse_pos, last_mouse_pos);
            seed_velocities[drag_seed_index] = vec2_div_scalar(delta_cursor, delta_time * 2.0f);
            last_mouse_pos = cur_mouse_pos;
        }
    } else {
        drag_seed_index = -1;
        last_mouse_pos = cur_mouse_pos;
    }
}

void _generate_voronoi_seeds() {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        seed_mark_radii[i] = SEED_RADIUS;
        _generate_seed_pos(i);
        _generate_seed_color(i);
        _generate_seed_velocity(i, lerpf(100, 300, rand_float()));
    }
}

void _generate_bubbles_seeds() {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        seed_mark_radii[i] = rand_float() * (SEED_MAX_RADIUS - SEED_MIN_RADIUS) + SEED_MIN_RADIUS;
        _generate_seed_pos(i);
        _generate_seed_color(i);
        _generate_seed_velocity(i, lerpf(100, 150, rand_float()));
    }
}

void _render_voronoi_frame(GLFWwindow* window, double delta_time, int width, int height) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _apply_constraints(width, height);
    _check_drag(window, height, delta_time);
    _solve_collisions();
    _update_positions(delta_time);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[ATTRIB_POS]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2) * SEED_COUNT, seed_positions);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, SEED_COUNT);
}

void _render_bubbles_frame(GLFWwindow* window, double delta_time, int width, int height) {
    (void)window;

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _apply_constraints(width, height);
    _solve_collisions();
    _update_positions(delta_time);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[ATTRIB_POS]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2) * SEED_COUNT, seed_positions);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, SEED_COUNT);
}