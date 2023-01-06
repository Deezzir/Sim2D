#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include "helpers.h"
#include "main.h"

bool pause = false;
Mode mode = MODE_VORONOI;
double global_delta_time = 0.0;

void (*generate_seeds)(void) = NULL;
void (*render_frame)(double, int, int) = NULL;

// Main function
int main(int argc, char** argv) {
    srand(time(0));

    init_mode(argc, argv);

    GLFWwindow* window;
    GLuint program;

    generate_seeds();

    init_glfw_settings();
    window = init_glfw_window();
    load_gl_extensions();
    init_glfw_callbacks(window);
    init_gl_settings();

    init_shaders(&program);
    glUseProgram(program);
    init_gl_uniforms(program);

    // ---------------------
    render_loop(window);
    // ---------------------

    glfwTerminate();
    return 0;
}

// Main loop
void render_loop(GLFWwindow* window) {
    double prev_time = 0.0;
    double delta_time = 0.0;

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        update_gl_uniforms(width, height);

        float sub_dt = delta_time / SUB_STEPS;
        for (size_t i = 0; i < SUB_STEPS; ++i) {
            render_frame(sub_dt, width, height);
            glfwSwapBuffers(window);
        }

        glfwPollEvents();

        double cur_time = glfwGetTime();
        delta_time = !pause ? cur_time - prev_time : global_delta_time;
        prev_time = cur_time;
    }
}

void init_mode(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--bubbles") == 0) {
            mode = MODE_BUBBLES;
        } else {
            fprintf(stderr, "ERROR: unknown flag `%s`\n", argv[i]);
            exit(1);
        }
    }

    switch (mode) {
        case MODE_VORONOI:
            generate_seeds = generate_voronoi_seeds;
            render_frame = render_voronoi_frame;
            break;
        case MODE_BUBBLES:
            generate_seeds = generate_bubbles_seeds;
            render_frame = render_bubbles_frame;
            break;
        default:
            UNREACHABLE("Unexpected execution mode");
    }

    assert(generate_seeds != NULL || "generate_seeds is NULL");
    assert(render_frame != NULL || "render_frame is NULL");

#if DEBUG
    printf("Running in %s mode\n", mode == MODE_VORONOI ? "Voronoi" : "Bubbles");
#endif
}