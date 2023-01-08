#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "main.h"

vec2 seed_positions[SEED_COUNT];
vec2 seed_velocities[SEED_COUNT];
vec4 seed_colors[SEED_COUNT];
GLint seed_mark_radii[SEED_COUNT];

int drag_seed_index = -1;
vec2 last_mouse_pos = {0.0f, 0.0f};

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

void _solve_collisions() {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        vec2 pos1 = seed_positions[i];

        for (size_t j = i + 1; j < SEED_COUNT; j++) {
            vec2 pos2 = seed_positions[j];
            float dist = dist_vec2(pos1, pos2);
            float radii_sum = seed_mark_radii[i] + seed_mark_radii[j];

            if (dist < radii_sum) {
                vec2 vel1 = seed_velocities[i];
                vec2 vel2 = seed_velocities[j];

                seed_velocities[i] = collision_sim(pos1, pos2, vel1, vel2, seed_mark_radii[i], seed_mark_radii[j]);
                seed_velocities[j] = collision_sim(pos2, pos1, vel2, vel1, seed_mark_radii[j], seed_mark_radii[i]);

                // Move the current seed apart
                float delta = radii_sum - dist;
                vec2  n =  vec2_div_scalar(vec2_sub(pos1, pos2), dist); 
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
            float dist = dist_vec2(pos1, pos2);
            float radii_sum = seed_mark_radii[i] + seed_mark_radii[j];

            if (dist <= radii_sum / 1.5f) {
                vec2 vel1 = seed_velocities[i];
                vec2 vel2 = seed_velocities[j];

                seed_velocities[i] = collision_sim(pos1, pos2, vel1, vel2, seed_mark_radii[i], seed_mark_radii[j]);
                seed_velocities[j] = collision_sim(pos2, pos1, vel2, vel1, seed_mark_radii[j], seed_mark_radii[i]);

                // Move the current seed apart
                float delta = radii_sum / 1.5f - dist;
                vec2  n =  vec2_div_scalar(vec2_sub(pos1, pos2), dist); 
                seed_positions[i] = vec2_add(seed_positions[i], vec2_mul_scalar(n, delta * 0.5f));
                seed_positions[j] = vec2_add(seed_positions[j], vec2_mul_scalar(n, delta * -0.5f));
            }
        }
    }
}

void _update_positions(double delta_time) {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        seed_positions[i] = vec2_add(seed_positions[i], vec2_mul_scalar(seed_velocities[i], delta_time));
    }
}

void _check_drag(GLFWwindow* window, double delta_time) {
    double xpos, ypos;
    int height;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwGetWindowSize(window, NULL, &height);
    vec2 cur_mouse_pos = (vec2) { (float)xpos, height - (float)ypos };

    if (drag_mode) {
        for (int i = 0; i < SEED_COUNT && drag_seed_index == -1; i++) {
            float dist = dist_vec2(seed_positions[i], cur_mouse_pos);
            if (dist < seed_mark_radii[i]) {
                drag_seed_index = i;
                break;
            }
        }

        if (drag_seed_index != -1) {
            seed_positions[drag_seed_index] = cur_mouse_pos;
            last_mouse_pos = cur_mouse_pos;
        }
    } else {
        if (drag_seed_index != -1) {
            vec2 cursor_movement = vec2_sub(cur_mouse_pos, last_mouse_pos);
            seed_velocities[drag_seed_index] = vec2_div_scalar(cursor_movement, delta_time);
            last_mouse_pos = cur_mouse_pos;
        }
        drag_seed_index = -1;
    }
}

void generate_voronoi_seeds() {

    for (size_t i = 0; i < SEED_COUNT; i++) {
        seed_positions[i].x = DEFAULT_SCREEN_WIDTH * rand_float() ;
        seed_positions[i].y = DEFAULT_SCREEN_HEIGHT * rand_float();

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
    for (size_t i = 0; i < SEED_COUNT; i++) {
        seed_positions[i].x = DEFAULT_SCREEN_WIDTH  / 2 + rand_float() * 100 - 50;
        seed_positions[i].y = DEFAULT_SCREEN_HEIGHT / 2 + rand_float() * 100 - 50;

        seed_colors[i].x = rand_float();
        seed_colors[i].y = rand_float();
        seed_colors[i].z = rand_float();
        seed_colors[i].w = 1.0f;

        // Generate random radius between BUBBLE_MIN_RADIUS and BUBBLE_MAX_RADIUS
        seed_mark_radii[i] = rand_float() * (BUBBLE_MAX_RADIUS - BUBBLE_MIN_RADIUS) + BUBBLE_MIN_RADIUS;

        float angle = rand_float() * 2.0f * M_PI;
        float mag = lerpf(100, 300, rand_float());
        seed_velocities[i].x = cosf(angle) * mag;
        seed_velocities[i].y = sinf(angle) * mag;
    }
}

void render_voronoi_frame(GLFWwindow* window, double delta_time, int width, int height) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _apply_constraints(width, height);
    _check_drag(window, delta_time);
    _solve_collisions();
    _update_positions(delta_time);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[ATTRIB_POS]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(seed_positions), seed_positions);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, SEED_COUNT);
}

void render_bubbles_frame(GLFWwindow* window, double delta_time, int width, int height) {
    (void)window;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _apply_constraints(width, height);
    _solve_collisions_bubbles();
    _update_positions(delta_time);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[ATTRIB_POS]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(seed_positions), seed_positions);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, SEED_COUNT);
}