#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include "glextloader.c"
#include "main.h"

// Main function
// ---------------------
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

    // RENDERING LOOP
    switch (mode) {
        case MODE_VORONOI:
            voronoi_loop(window);
            break;
        case MODE_BUBBLES:
            bubbles_loop(window);
            break;
        default:
            UNREACHABLE("Unexpected execution mode");
    }

    glfwTerminate();
    return 0;
}

// Functions definitions
// ---------------------
void init_mode(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--bubbles") == 0) {
            mode = MODE_BUBBLES;
        } else {
            fprintf(stderr, "ERROR: unknown flag `%s`\n", argv[i]);
            exit(1);
        }
    }
#if DEBUG
    printf("Running in %s mode\n", mode == MODE_VORONOI ? "Voronoi" : "Bubbles");
#endif
}

// Utility functions
const char* shader_type_as_cstr(GLuint shader) {
    switch (shader) {
        case GL_VERTEX_SHADER:
            return "GL_VERTEX_SHADER";
        case GL_FRAGMENT_SHADER:
            return "GL_FRAGMENT_SHADER";
        default:
            return "(Unknown)";
    }
}

char* slurp_file_into_malloced_cstr(const char* file_path) {
    FILE* f = NULL;
    char* buffer = NULL;

    f = fopen(file_path, "r");
    if (f == NULL) goto fail;
    if (fseek(f, 0, SEEK_END) < 0) goto fail;

    long size = ftell(f);
    if (size < 0) goto fail;

    buffer = malloc(size + 1);
    if (buffer == NULL) goto fail;

    if (fseek(f, 0, SEEK_SET) < 0) goto fail;

    int ret = fread(buffer, 1, size, f);
    if (ferror(f) || ret == 0) goto fail;

    buffer[size] = '\0';

    if (f) {
        fclose(f);
        errno = 0;
    }
    return buffer;
fail:
    if (f) {
        int saved_errno = errno;
        fclose(f);
        errno = saved_errno;
    }
    if (buffer) {
        free(buffer);
    }
    return NULL;
}

bool compile_shader_source(const GLchar* source, GLenum shader_type, GLuint* shader) {
    *shader = glCreateShader(shader_type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);

        fprintf(stderr, "[ERROR]: Could not compile %s\n", shader_type_as_cstr(shader_type));
        fprintf(stderr, "%.*s\n", message_size, message);
        return false;
    }

    return true;
}

bool compile_shader_file(const char* file_path, GLenum shader_type, GLuint* shader) {
    char* source = slurp_file_into_malloced_cstr(file_path);
    if (source == NULL) {
        fprintf(stderr, "[ERROR]: Failed to read file file `%s`: %s\n", file_path, strerror(errno));
        errno = 0;
        return false;
    }
    bool ok = compile_shader_source(source, shader_type, shader);
    if (!ok) {
        fprintf(stderr, "[ERROR]: Failed to compile `%s` shader file\n", file_path);
    }
    free(source);
    return ok;
}

bool link_program(GLuint vert_shader, GLuint frag_shader, GLuint* program) {
    *program = glCreateProgram();

    glAttachShader(*program, vert_shader);
    glAttachShader(*program, frag_shader);
    glLinkProgram(*program);

    GLint linked = 0;
    glGetProgramiv(*program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLchar message[1024];
        GLsizei message_size = 0;

        glGetProgramInfoLog(*program, sizeof(message), &message_size, message);
        fprintf(stderr, "[ERROR]: Program Linking: %.*s\n", message_size, message);
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    return program;
}

bool load_shader_program(const char* vertex_file_path, const char* fragment_file_path, GLuint* program) {
    GLuint vert = 0;
    if (!compile_shader_file(vertex_file_path, GL_VERTEX_SHADER, &vert)) {
        return false;
    }

    GLuint frag = 0;
    if (!compile_shader_file(fragment_file_path, GL_FRAGMENT_SHADER, &frag)) {
        return false;
    }

    if (!link_program(vert, frag, program)) {
        return false;
    }

    return true;
}

float rand_float() {
    return (float)rand() / (float)RAND_MAX;
}

float lerpf(float start, float end, float t) {
    return start + (end - start) * t;
}

// GLFW helpers
void init_glfw_settings(void) {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "[ERROR]: Could not initialize GLFW\n");
        exit(1);
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
}

GLFWwindow* init_glfw_window() {
    GLFWwindow* window = glfwCreateWindow(
        DEFAULT_SCREEN_WIDTH,
        DEFAULT_SCREEN_HEIGHT,
        "Voronoi",
        NULL,
        NULL);

    if (window == NULL) {
        fprintf(stderr, "[ERROR]; Could not create a window\n");
        glfwTerminate();
        exit(1);
    }

    int gl_ver_major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    int gl_ver_minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    printf("OpenGL %d.%d\n", gl_ver_major, gl_ver_minor);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    return window;
}

void init_glfw_callbacks(GLFWwindow* window) {
#ifdef DEBUG
    if (glDebugMessageCallback != NULL && glDebugMessageControl != NULL) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(message_callback, 0);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    }
#endif
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, window_resize_callback);
}

void init_gl_settings() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(COUNT_ATTRIBS, vbos);
    {
        glGenBuffers(1, &vbos[ATTRIB_POS]);
        glBindBuffer(GL_ARRAY_BUFFER, vbos[ATTRIB_POS]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(seed_positions), seed_positions, GL_STATIC_DRAW);

        glEnableVertexAttribArray(ATTRIB_POS);
        glVertexAttribPointer(ATTRIB_POS,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              0,
                              (void*)0);
        glVertexAttribDivisor(ATTRIB_POS, 1);
    }
    {
        glGenBuffers(1, &vbos[ATTRIB_COLOR]);
        glBindBuffer(GL_ARRAY_BUFFER, vbos[ATTRIB_COLOR]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(seed_colors), seed_colors, GL_STATIC_DRAW);

        glEnableVertexAttribArray(ATTRIB_COLOR);
        glVertexAttribPointer(ATTRIB_COLOR,
                              4,
                              GL_FLOAT,
                              GL_FALSE,
                              0,
                              (void*)0);
        glVertexAttribDivisor(ATTRIB_COLOR, 1);
    }
    {
        glGenBuffers(1, &vbos[ATTRIB_RADIUS]);
        glBindBuffer(GL_ARRAY_BUFFER, vbos[ATTRIB_RADIUS]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(seed_mark_radii), seed_mark_radii, GL_STATIC_DRAW);

        glEnableVertexAttribArray(ATTRIB_RADIUS);
        glVertexAttribIPointer(ATTRIB_RADIUS,
                              1,
                              GL_INT,
                              0,
                              (void*)0);
        glVertexAttribDivisor(ATTRIB_RADIUS, 1);
    }
}

void init_shaders(GLuint* program) {
    const char* vertex_path = VERTEX_FILE_PATH;
    const char* fragment_path;

    switch (mode) {
        case MODE_VORONOI:
            fragment_path = VORONOI_FRAGMENT_FILE_PATH;
            break;
        case MODE_BUBBLES:
            fragment_path = BUBBLES_FRAGMENT_FILE_PATH;
            break;
        default:
            UNREACHABLE("Unexpected execution mode");
    }

    if (!load_shader_program(vertex_path, fragment_path, program)) {
        exit(1);
    }
}

void init_gl_uniforms(GLuint program) {
    for (Uniform i = 0; i < COUNT_UNIFORMS; i++) {
        uniforms[i] = glGetUniformLocation(program, uniform_names[i]);
    }

    glUniform2f(uniforms[RESOLUTION_UNIFORM], DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);
    glUniform4f(uniforms[SEED_MARK_COLOR_UNIFORM], SEED_MARK_COLOR.x, SEED_MARK_COLOR.y, SEED_MARK_COLOR.z, SEED_MARK_COLOR.w);
}

void update_gl_uniforms(int width, int height) {
    glUniform2f(uniforms[RESOLUTION_UNIFORM], width, height);
}

// Callbacks
void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                      GLsizei length, const GLchar* message, const void* userParam) {
    (void)source;
    (void)id;
    (void)length;
    (void)userParam;

    fprintf(stderr, "GL CALLBACK: %s source = 0x%x, type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            source, type, severity, message);
}

void window_resize_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)action;
    (void)mods;
    (void)window;

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_SPACE) {
            pause = !pause;
        } else if (key == GLFW_KEY_Q) {
            exit(1);
        }

        if (pause) {
            if (key == GLFW_KEY_LEFT) {
                global_delta_time = -MANUAL_TIME_STEP;
            } else if (key == GLFW_KEY_RIGHT) {
                global_delta_time = MANUAL_TIME_STEP;
            }
        }
    }

    if (action == GLFW_RELEASE) {
        global_delta_time = !(key == GLFW_KEY_LEFT && global_delta_time < 0.0f) * global_delta_time;
        global_delta_time = !(key == GLFW_KEY_RIGHT && global_delta_time > 0.0f) * global_delta_time;
    }
}

// Voronoi helpers
void generate_seeds() {
    for (size_t i = 0; i < SEED_COUNT; i++) {
        seed_positions[i].x = rand_float() * DEFAULT_SCREEN_WIDTH;
        seed_positions[i].y = rand_float() * DEFAULT_SCREEN_HEIGHT;

        seed_colors[i].x = rand_float();
        seed_colors[i].y = rand_float();
        seed_colors[i].z = rand_float();
        seed_colors[i].w = 1.0f;

        seed_mark_radii[i] = mode == MODE_BUBBLES ? 
            rand_float() * (SEED_MARK_MAX_RADIUS - SEED_MARK_MIN_RADIUS) + SEED_MARK_MIN_RADIUS : 
            SEED_MARK_RADIUS
        ;

        float angle = rand_float() * 2.0f * M_PI;
        float mag = lerpf(100, 200, rand_float());
        seed_velocities[i].x = cosf(angle) * mag;
        seed_velocities[i].y = sinf(angle) * mag;
    }
}

// Rendering
void render_voronoi_frame(double delta_time, int width, int height) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (size_t i = 0; i < SEED_COUNT; ++i) {
        float x = seed_positions[i].x + seed_velocities[i].x * delta_time;
        float y = seed_positions[i].y + seed_velocities[i].y * delta_time;
        if (x < 0.0f || x > width) {
            seed_velocities[i].x *= -1.0f;
        }
        if (y < 0.0f || y > height) {
            seed_velocities[i].y *= -1.0f;
        }
        seed_positions[i].y = y;
        seed_positions[i].x = x;
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbos[ATTRIB_POS]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(seed_positions), seed_positions);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, SEED_COUNT);
}

void bubbles_loop(GLFWwindow* window) {
    double prev_time = 0.0;
    double delta_time = 0.0;

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        update_gl_uniforms(width, height);

        render_voronoi_frame(delta_time, width, height);

        glfwSwapBuffers(window);
        glfwPollEvents();

        double cur_time = glfwGetTime();
        delta_time = !pause ? cur_time - prev_time : global_delta_time;

        prev_time = cur_time;
    }
}

void voronoi_loop(GLFWwindow* window) {
    double prev_time = 0.0;
    double delta_time = 0.0;

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        update_gl_uniforms(width, height);

        render_voronoi_frame(delta_time, width, height);

        glfwSwapBuffers(window);
        glfwPollEvents();

        double cur_time = glfwGetTime();
        delta_time = !pause ? cur_time - prev_time : global_delta_time;

        prev_time = cur_time;
    }
}