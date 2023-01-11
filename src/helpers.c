#include "helpers.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

// This source inner helpers
void _invalid_arg_exit();

float _sqr_dist(float x1, float y1, float x2, float y2);
float _dist(float x1, float y1, float x2, float y2);
float _dot(float x1, float y1, float x2, float y2);
bool _is_in_range(int target, int min, int max);
int _options(int argc, char *argv[], const char *legal);

const char *legal_args = "m:c:r:";
const char switch_char = '-';
const char unknown_char = '?';
char *opt_arg = NULL;
int opt_index = 1;

// Function definitions
// ---------------------
void usage(void) {
    printf("usage: sim [-m num] [-c num] [-r num]\n");
    printf("       Optionally specify simulation mode: [-m] (%u-%u). By default Mode 1 is chosen\n", 1, COUNT_MODES);
    printf("              Mode 1: - 'Voronoi'\n");
    printf("              Mode 2: - 'Atoms'\n");
    printf("              Mode 3: - 'Bubbles'\n");
    printf("       Optionally specify seed count:      [-c] (%u-%u)\n", 1, SEED_MAX_COUNT);
    printf("       Optionally specify seed radius:     [-r] (%u-%u). Only works with 'voronoi' and 'atoms' modes\n", SEED_MIN_RADIUS, SEED_MAX_RADIUS);
}

void get_arguments(int argc, char **argv) {
    int letter = -1;
    int value = -1;
    char *tail = "\n";

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "--help") == 0) {
                usage();
                exit(0);
            }
        }

        // Precheck
        letter = _options(argc, argv, legal_args);
        value = opt_arg != NULL ? (int)strtoul(opt_arg, &tail, 10) : -1;

        while (letter != -1) {
            if (letter == unknown_char) {
                printf("invalid option: %s\n", argv[opt_index - 1]);
                _invalid_arg_exit();
            } else if (errno == ERANGE) {
                printf("invalid option argument: [-%c num] - number is invalid\n", letter);
                _invalid_arg_exit();
            } else if (value == -1 || tail[0] != '\0' || errno == EINVAL) {
                printf("invalid option argument: [-%c num] - must be followed by number\n", letter);
                _invalid_arg_exit();
            }

            switch (letter) {
                case 'm':
                    if (_is_in_range(value, 1, COUNT_MODES))
                        SIM_MODE = value - 1;
                    else {
                        printf("for 'mode' option [-%c]\n", letter);
                        _invalid_arg_exit();
                    }
                    break;
                case 'c':
                    if (_is_in_range(value, 1, SEED_MAX_COUNT))
                        SEED_COUNT = value;
                    else {
                        printf("for 'count' option [-%c]\n", letter);
                        _invalid_arg_exit();
                    }
                    break;
                case 'r':
                    if (_is_in_range(value, SEED_MIN_RADIUS, SEED_MAX_RADIUS))
                        SEED_RADIUS = value;
                    else {
                        printf("for 'radius' option [-%c]\n", letter);
                        _invalid_arg_exit();
                    }
                    break;
                default:
                    break;
            }

            tail = "\0";
            letter = _options(argc, argv, legal_args);
            value = opt_arg != NULL ? (int)strtoul(opt_arg, &tail, 10) : -1;
        }
    }
}

float rand_float(void) {
    return (float)rand() / (float)RAND_MAX;
}

float lerpf(float start, float end, float t) {
    return start + (end - start) * t;
}

float vec2_sqr_dist(vec2 v1, vec2 v2) {
    return _sqr_dist(v1.x, v1.y, v2.x, v2.y);
}

float vec2_dist(vec2 v1, vec2 v2) {
    return _dist(v1.x, v1.y, v2.x, v2.y);
}

float vec2_sqr_mag(vec2 v) {
    return _sqr_dist(v.x, v.y, 0.0f, 0.0f);
}

float vec2_mag(vec2 v) {
    return _dist(v.x, v.y, 0.0f, 0.0f);
}

float vec2_dot(vec2 v1, vec2 v2) {
    return _dot(v1.x, v1.y, v2.x, v2.y);
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

vec2 vec2_scale(vec2 v, float s) {
    return (vec2){v.x * s, v.y * s};
}

vec2 vec2_div_scalar(vec2 v, float s) {
    assert(s != 0.0f);
    return (vec2){v.x / s, v.y / s};
}

vec2 vec2_normalize(vec2 v) {
    float mag = vec2_mag(v);
    assert(mag != 0.0f);

    return (vec2){v.x / mag, v.y / mag};
}

vec2 vec2_unit_normal(vec2 v1, vec2 v2) {
    float dist = vec2_dist(v1, v2);
    vec2 normal = vec2_div_scalar(vec2_sub(v2, v1), dist);
    return normal;
}

vec2 vec2_unit_tangent(vec2 v1, vec2 v2) {
    vec2 tangent;
    tangent.x = -vec2_unit_normal(v1, v2).y;
    tangent.y = vec2_unit_normal(v1, v2).x;
    return tangent;
}

bool vec2_is_zero(vec2 v) {
    return v.x == 0.0f && v.y == 0.0f;
}

void collision_sim_0(vec2 pos1, vec2 pos2, float m1, float m2, vec2 *vel1, vec2 *vel2) {
    assert(m1 > 0.0f && m2 > 0.0f);
    assert(vec2_sqr_dist(pos1, pos2) > 0.0f);

    // new_vel1 = vel1 - (2 * m2 / (m1 + m2)) * ((vel1 - vel2) • (pos1 - pos2)) / ||pos1 - pos2||^2 * (pos1 - pos2)
    // new_vel2 = vel2 - (2 * m1 / (m1 + m2)) * ((vel2 - vel1) • (pos2 - pos1)) / ||pos2 - pos1||^2 * (pos2 - pos1)
    vec2 pos1_diff = vec2_sub(pos1, pos2);
    vec2 vel1_diff = vec2_sub(*vel1, *vel2);
    vec2 pos2_diff = vec2_sub(pos2, pos1);
    vec2 vel2_diff = vec2_sub(*vel2, *vel1);

    float new_vel1_mag = (2.0f * m2) / (m1 + m2) * vec2_dot(vel1_diff, pos1_diff) / vec2_sqr_mag(pos1_diff);
    float new_vel2_mag = (2.0f * m1) / (m1 + m2) * vec2_dot(vel2_diff, pos2_diff) / vec2_sqr_mag(pos2_diff);

    *vel1 = vec2_sub(*vel1, vec2_scale(pos1_diff, new_vel1_mag));
    *vel2 = vec2_sub(*vel2, vec2_scale(pos2_diff, new_vel2_mag));
}

void collision_sim_1(vec2 pos1, vec2 pos2, float m1, float m2, vec2 *vel1, vec2 *vel2) {
    assert(m1 > 0.0f && m2 > 0.0f);
    assert(vec2_sqr_dist(pos1, pos2) > 0.0f);

    // Calculate unit normal and unit tangent
    vec2 un = vec2_unit_normal(pos1, pos2);
    vec2 ut = vec2_unit_tangent(pos1, pos2);

    // Calculate scalar velocity along unit normal and unit tangent
    float v1n = vec2_dot(*vel1, un);
    float v1t = vec2_dot(*vel1, ut);
    float v2n = vec2_dot(*vel2, un);
    float v2t = vec2_dot(*vel2, ut);

    // Calculate new normal velocities
    float v1n_new = (v1n * (m1 - m2) + 2 * m2 * v2n) / (m1 + m2);
    float v2n_new = (v2n * (m2 - m1) + 2 * m1 * v1n) / (m2 + m1);

    // Calculate new tangential velocities
    float v1t_new = v1t;
    float v2t_new = v2t;

    // Calculate new velocities in the normal and tangential directions
    *vel1 = vec2_add(vec2_scale(un, v1n_new), vec2_scale(ut, v1t_new));
    *vel2 = vec2_add(vec2_scale(un, v2n_new), vec2_scale(ut, v2t_new));
}

float _sqr_dist(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return dx * dx + dy * dy;
}

// Private function definitions
// ---------------------
void _invalid_arg_exit(void) {
    usage();
    exit(EINVAL);
}

float _dist(float x1, float y1, float x2, float y2) {
    return sqrtf(_sqr_dist(x1, y1, x2, y2));
}

float _dot(float x1, float y1, float x2, float y2) {
    return x1 * x2 + y1 * y2;
}

bool _is_in_range(int target, int min, int max) {
    if (target > max || target < min) {
        printf("provided argument: %d is not in range (%d-%d) ", target, min, max);
        return false;
    }
    return true;
}

int _options(int argc, char *argv[], const char *legal) {
    static char *posn = "";  // position in argv[opt_index]
    char *legal_index = NULL;
    int letter = 0;

    if (!*posn) {
        // no more args, no switch_char or no option letter ?
        if ((opt_index >= argc) ||
            (*(posn = argv[opt_index]) != switch_char) ||
            !*++posn)
            return -1;
        // find double switch_char ?
        if (*posn == switch_char) {
            opt_index++;
            return -1;
        }
    }
    letter = *posn++;
    if (!(legal_index = strchr(legal, letter))) {
        if (!*posn)
            opt_index++;
        return unknown_char;
    }
    if (*++legal_index != ':') {
        // no option arg
        opt_arg = NULL;
        if (!*posn)
            opt_index++;
    } else {
        if (*posn)
            // no space between opt and opt arg
            opt_arg = posn;
        else if (argc <= ++opt_index) {
            posn = "";
            opt_arg = NULL;
        } else
            opt_arg = argv[opt_index];
        posn = "";
        opt_index++;
    }
    return letter;
}