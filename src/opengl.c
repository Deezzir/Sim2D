#include "main.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


const char* uniform_names[COUNT_UNIFORMS] = {
    [RESOLUTION_UNIFORM] = "resolution",
    [SEED_MARK_COLOR_UNIFORM] = "seed_mark_color",
    [SEED_MARK_RADIUS_UNIFORM] = "seed_mark_radius",
};

GLuint vbos[COUNT_ATTRIBS];
GLint uniforms[COUNT_UNIFORMS];

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

    GLuint vao;
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