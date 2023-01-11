#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"

#define GRAVITY ((vec2){0.0f, -20.0f})
#define GRID_SIZE 32
#define NUM_BUCKETS (GRID_SIZE * GRID_SIZE)

static_assert(COUNT_MODES == 3, "Update list of mode names");
const char* mode_names[COUNT_MODES] = {
    [MODE_VORONOI] = "Voronoi",
    [MODE_ATOMS] = "Atoms",
    [MODE_BUBBLES] = "Bubbles",
};

// This source inner helpers
size_t _hash(float x, float y);
void _map_insert(Seed* s);
void _map_remove(Seed* s);

void _allocate_memory(void);

void _generate_seed_pos(Seed* s);
void _generate_seed_color(Seed* s);
void _generate_seed_dynamics(Seed* s, vec2 acc, float mag);

void _apply_constraints(int width, int height);
void _apply_gravity(void);
void _check_drag(GLFWwindow* window, int height, double dt);
void _update_positions(double dt);

void _find_collisions(Seed* s, float collision_dist, Seed** candidates, size_t* count);
void _solve_collisions_voronoi(void);
void _solve_collisions_bubbles(void);

void _generate_voronoi_seeds(void);
void _generate_bubbles_seeds(void);
void _render_voronoi_frame(GLFWwindow* window, double dt, int width, int height);
void _render_bubbles_frame(GLFWwindow* window, double dt, int width, int height);

typedef struct Bucket {
    Seed* seed;
    struct Bucket* next;
    struct Bucket* prev;
} Bucket;

typedef struct {
    float k;
    float rest_len;
    Seed* p0;
    Seed* p1;
} Spring;

int SEED_RADIUS = DEFAULT_SEED_RADIUS;
size_t SEED_COUNT = DEFAULT_SEED_COUNT;

Bucket* pos_map[NUM_BUCKETS];
Seed* seeds = NULL;

Seed* drag_seed = NULL;
vec2 last_mouse_pos = {0.0f, 0.0f};
void (*_solve_collisions)(void) = NULL;

// Function definitions
// ---------------------
void init_sim_mode(Mode mode) {
    _allocate_memory();

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

void free_sim_mode(void) {
    free(seeds);
    seeds = NULL;

    for (int i = 0; i < NUM_BUCKETS; i++) {
        Bucket* b = pos_map[i];
        while (b != NULL) {
            Bucket* b_next = b->next;
            free(b);
            b = b_next;
        }
    }
}

// Private function definitions
// ---------------------
size_t _hash(float x, float y) {
    return (size_t)(x / GRID_SIZE + y / GRID_SIZE) % NUM_BUCKETS;
}

void _map_insert(Seed* s) {
    size_t idx = _hash(s->pos.x, s->pos.y);

    Bucket* b = malloc(sizeof(Bucket));
    b->seed = s;
    b->prev = NULL;

    b->next = pos_map[idx];
    if (b->next != NULL)
        b->next->prev = b;
    pos_map[idx] = b;
}

void _map_remove(Seed* s) {
    size_t idx = _hash(s->pos.x, s->pos.y);

    Bucket* b = pos_map[idx];
    while (b != NULL) {
        if (b->seed == s) {
            if (b->prev != NULL) {
                b->prev->next = b->next;
            } else {
                pos_map[idx] = b->next;
            }

            if (b->next != NULL) {
                b->next->prev = b->prev;
            }
            free(b);
            break;
        }

        b = b->next;
    }
}

void _allocate_memory(void) {
    for (int i = 0; i < NUM_BUCKETS; i++) {
        pos_map[i] = NULL;
    }

    if (seeds != NULL)
        free_sim_mode();

    seeds = (Seed*)calloc(SEED_COUNT, sizeof(Seed));

    if (seeds == NULL) {
        printf("[ERROR]: Memory was not allocated\n");
        exit(EXIT_FAILURE);
    }
}

void _generate_seed_pos(Seed* s) {
    s->pos.x = DEFAULT_SCREEN_WIDTH / 2 + rand_float() * 100 - 50;
    s->pos.y = DEFAULT_SCREEN_HEIGHT / 2 + rand_float() * 100 - 50;
}

void _generate_seed_color(Seed* s) {
    s->color.x = rand_float();
    s->color.y = rand_float();
    s->color.z = rand_float();
    s->color.w = 1.0f;
}

void _generate_seed_dynamics(Seed* s, vec2 acc, float mag) {
    float angle = rand_float() * 2.0f * M_PI;
    s->vel.x = cosf(angle) * mag;
    s->vel.y = sinf(angle) * mag;

    s->acc = acc;
}

void _apply_constraints(int width, int height) {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        Seed* s = &seeds[i];

        // Bounce off walls
        if ((s->pos.x < 0.0f && s->vel.x < 0) || (s->pos.x > width && s->vel.x > 0)) {
            s->vel = vec2_mul(s->vel, (vec2){-1.0f, 1.0f});
        }
        if ((s->pos.y < 0.0f && s->vel.y < 0) || (s->pos.y > height && s->vel.y > 0)) {
            s->vel = vec2_mul(s->vel, (vec2){1.0f, -1.0f});
        }
    }
}

void _apply_gravity(void) {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        Seed* s = &seeds[i];
        s->acc = vec2_add(s->acc, GRAVITY);
    }
}

void _find_collisions(Seed* s, float collision_dist, Seed** candidates, size_t* count) {
    *count = 0;
    bool visited[NUM_BUCKETS] = {0};
    int min_x = (int)((s->pos.x - collision_dist) / GRID_SIZE);
    int min_y = (int)((s->pos.y - collision_dist) / GRID_SIZE);
    int max_x = (int)((s->pos.x + collision_dist) / GRID_SIZE);
    int max_y = (int)((s->pos.y + collision_dist) / GRID_SIZE);

    for (int x = min_x; x <= max_x; x++) {
        for (int y = min_y; y <= max_y; y++) {
            size_t idx = _hash(x * GRID_SIZE, y * GRID_SIZE);
            if (visited[idx]) continue;
            visited[idx] = true;
            Bucket* b = pos_map[idx];

            while (b != NULL) {
                float dist = vec2_dist(s->pos, b->seed->pos);
                if (dist < collision_dist) {
                    candidates[(*count)++] = b->seed;
                }
                b = b->next;
            }
        }
    }
}

void _solve_collisions_voronoi(void) {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        Seed* s1 = &seeds[i];

        _map_remove(s1);
        Seed* candidates[SEED_COUNT];
        size_t cand_count = 0;
        _find_collisions(s1, 3.0f * s1->radius, candidates, &cand_count);

        for (size_t j = 0; j < cand_count; j++) {
            Seed* s2 = candidates[j];

            float dist = vec2_dist(s1->pos, s2->pos);
            float radii_sum = s1->radius + s2->radius;

            if (dist < radii_sum) {
                _map_remove(s2);

                int rad1 = s1->radius;
                int rad2 = s2->radius;
                int c1 = s1 == drag_seed;
                int c2 = s2 == drag_seed;
                rad1 = INT_MAX * (c1 && vec2_is_zero(s1->vel)) + rad1 * !(c1 && vec2_is_zero(s1->vel));
                rad2 = INT_MAX * (c2 && vec2_is_zero(s2->vel)) + rad2 * !(c2 && vec2_is_zero(s2->vel));

                collision_sim_1(s1->pos, s2->pos, rad1, rad2, &s1->vel, &s2->vel);

                // Move the current seed apart
                float delta = radii_sum - dist;
                vec2 n = vec2_scale(vec2_sub(s1->pos, s2->pos), 1 / dist);
                float delta1 = !(c1 || c2) * (delta * 0.5f) + (c1 || c2) * delta;
                float delta2 = delta1 * -1.0f;
                s1->pos = vec2_add(s1->pos, vec2_scale(n, delta1));
                s2->pos = vec2_add(s2->pos, vec2_scale(n, delta2));

                _map_insert(s2);
            }
        }
        _map_insert(s1);
    }
}

void _solve_collisions_bubbles(void) {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        Seed* s1 = &seeds[i];

        for (size_t j = i + 1; j < SEED_COUNT; j++) {
            Seed* s2 = &seeds[j];

            float dist = vec2_dist(s1->pos, s2->pos);
            float radii_sum = s1->radius + s2->radius;

            if (dist < radii_sum / 1.5f) {

                collision_sim_1(s1->pos, s2->pos, s1->radius, s2->radius, &s1->vel, &s2->vel);

                // Move the current seed apart
                float delta = radii_sum / 1.5f - dist;
                vec2 n = vec2_scale(vec2_sub(s1->pos, s2->pos), 1 / dist);
                s1->pos = vec2_add(s1->pos, vec2_scale(n, delta * 0.5f));
                s2->pos = vec2_add(s2->pos, vec2_scale(n, delta * -0.5f));
            }
        }
    }
}

void _update_positions(double dt) {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        Seed* s = &seeds[i];

        if (drag_seed != s) {
            _map_remove(s);
            s->vel = vec2_add(s->vel, vec2_scale(s->acc, dt));
            s->pos = vec2_add(s->pos, vec2_scale(s->vel, dt));
            s->acc = (vec2){0.0f, 0.0f};
            _map_insert(s);
        }
    }
}

void _check_drag(GLFWwindow* window, int height, double dt) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    vec2 cur_mouse_pos = (vec2){(float)xpos, height - (float)ypos};

    if (IS_DRAG_MODE) {
        for (size_t i = 0; i < SEED_COUNT && drag_seed == NULL; i++) {
            float dist = vec2_dist(seeds[i].pos, cur_mouse_pos);
            if (dist < seeds[i].radius) {
                drag_seed = &seeds[i];
                break;
            }
        }

        if (drag_seed != NULL) {            
            _map_remove(drag_seed);
            drag_seed->pos = cur_mouse_pos;
            _map_insert(drag_seed);

            vec2 delta_cursor = vec2_sub(cur_mouse_pos, last_mouse_pos);
            drag_seed->vel = vec2_scale(delta_cursor, 1 / (dt * 2.0f));
            last_mouse_pos = cur_mouse_pos;
        }
    } else {
        drag_seed = NULL;
        last_mouse_pos = cur_mouse_pos;
    }
}

void _generate_voronoi_seeds(void) {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        Seed* s = &seeds[i];

        s->radius = SEED_RADIUS;

        _generate_seed_pos(s);
        _generate_seed_color(s);
        _generate_seed_dynamics(s, (vec2){0.0f, 0.0f}, lerpf(100, 300, rand_float()));

        _map_insert(s);
    }
}

void _generate_bubbles_seeds(void) {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        Seed* s = &seeds[i];

        s->radius = rand_float() * (SEED_MAX_RADIUS - SEED_MIN_RADIUS + 20) + SEED_MIN_RADIUS + 20;

        _generate_seed_pos(s);
        _generate_seed_color(s);
        _generate_seed_dynamics(s, GRAVITY, lerpf(100, 150, rand_float()));

        _map_insert(s);
    }
}

void _render_voronoi_frame(GLFWwindow* window, double dt, int width, int height) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _apply_constraints(width, height);
    _check_drag(window, height, dt);
    _solve_collisions();
    _update_positions(dt);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(seeds[0]) * SEED_COUNT, seeds);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, SEED_COUNT);
}

void _render_bubbles_frame(GLFWwindow* window, double dt, int width, int height) {
    UNUSED(window);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _apply_constraints(width, height);
    _apply_gravity();
    _solve_collisions();
    _update_positions(dt);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(seeds[0]) * SEED_COUNT, seeds);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, SEED_COUNT);
}