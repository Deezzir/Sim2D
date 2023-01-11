#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include "helpers.h"
#include "main.h"

void init_signal_handler(void);
void _signal_handler(int signal);
void _exit_handler(void);

Mode SIM_MODE = MODE_VORONOI;
double DELTA_TIME = 0.0;

bool IS_PAUSE = false;
bool IS_DRAG_MODE = false;
bool IS_RUNNING = false;

void (*render_frame)(GLFWwindow*, double, int, int) = NULL;

// Main function
int main(int argc, char** argv) {
    srand(time(0));

    get_arguments(argc, argv);
    init_signal_handler();
    atexit(_exit_handler);

    init_sim_mode(SIM_MODE);

    GLFWwindow* window;
    GLuint program;

    init_glfw_settings();
    window = init_glfw_window();
    load_gl_extensions();
    init_glfw_callbacks(window);
    init_gl_settings();

    init_shaders(&program,
                 vertex_files[GENERAL_VERTEX],
                 fragment_files[SIM_MODE]);

    glUseProgram(program);
    init_gl_uniforms(program);

    // ---------------------
    render_loop(window);
    // ---------------------

    return 0;
}

// Main loop
void render_loop(GLFWwindow* window) {
    IS_RUNNING = true;

    double prev_time = 0.0;
    double dt = 0.0;

    int prev_width = DEFAULT_SCREEN_WIDTH;
    int prev_height = DEFAULT_SCREEN_HEIGHT;
    int width, height;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    while (!glfwWindowShouldClose(window) && IS_RUNNING) {
        glfwGetWindowSize(window, &width, &height);
        if (width != prev_width || height != prev_height) {
            prev_width = width;
            prev_height = height;
            update_gl_uniforms(width, height);
        }

        float sub_dt = dt / SUB_STEPS;
        for (size_t i = 0; i < SUB_STEPS; ++i) {
            render_frame(window, sub_dt, width, height);
            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        double cur_time = glfwGetTime();
        dt = !IS_PAUSE ? cur_time - prev_time : DELTA_TIME;
        prev_time = cur_time;
    }
}

void init_signal_handler(void) {
    struct sigaction action;
    action.sa_handler = &_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        printf("[ERROR]: Failed to set signal action");
        exit(EXIT_FAILURE);
    }
}

void _signal_handler(int signal) {
    switch (signal) {
        case SIGINT:
            IS_RUNNING = false;
            break;
        default:
            break;
    }
}

void _exit_handler(void) {
    printf("[INFO]: Goodbye, stranger!\n");
    free_sim_mode();
    glfwTerminate();
}