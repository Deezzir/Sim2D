#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "main.h"

// This source inner helpers
const char* _shader_type_as_cstr(GLuint shader);
char* _slurp_file_into_malloced_cstr(const char* file_path);
bool _compile_shader_source(const GLchar* source, GLenum shader_type, GLuint* shader);
bool _compile_shader_file(const char* file_path, GLenum shader_type, GLuint* shader);
bool _link_program(GLuint vert_shader, GLuint frag_shader, GLuint* program);
bool _load_shader_program(const char* vertex_file_path, const char* fragment_file_path, GLuint* program);

void _message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
void _window_resize_callback(GLFWwindow* window, int width, int height);
void _key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void _mouse_callback(GLFWwindow* window, int button, int action, int mods);

static_assert(COUNT_UNIFORMS == 1, "Update list of uniform names");
const char* uniform_names[COUNT_UNIFORMS] = {
    [RESOLUTION_UNIFORM] = "resolution",
};

static_assert(COUNT_VERTICES == 1, "Update list of vertex file paths");
const char* vertex_files[COUNT_VERTICES] = {
    [GENERAL_VERTEX] =  VERTEX_FILE_PATH,
};

static_assert(COUNT_FRAGMENTS == 3, "Update list of fragment file paths");
const char* fragment_files[COUNT_FRAGMENTS] = {
    [VORONOI_FRAGMENT] = VORONOI_FRAGMENT_FILE_PATH,
    [ATOMS_FRAGMENT] = ATOMS_FRAGMENT_FILE_PATH,
    [BUBBLES_FRAGMENT] = BUBBLES_FRAGMENT_FILE_PATH,
};

GLuint vbo = 0;
GLuint vao = 0;
GLint uniforms[COUNT_UNIFORMS];

// Function definitions
// ---------------------
void init_gl_uniforms(GLuint program) {
    for (Uniform i = 0; i < COUNT_UNIFORMS; i++) {
        uniforms[i] = glGetUniformLocation(program, uniform_names[i]);
    }

    glUniform2f(uniforms[RESOLUTION_UNIFORM], DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);
}

void update_gl_uniforms(int width, int height) {
    glUniform2f(uniforms[RESOLUTION_UNIFORM], width, height);
}

void init_glfw_settings(void) {
    if (!glfwInit()) {
        fprintf(stderr, "[ERROR]: Could not initialize GLFW\n");
        exit(1);
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
}

GLFWwindow* init_glfw_window(void) {
    GLFWwindow* window = glfwCreateWindow(
        DEFAULT_SCREEN_WIDTH,
        DEFAULT_SCREEN_HEIGHT,
        "Sim2D",
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
    printf("GLFW %s\n", glfwGetVersionString());

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    return window;
}

void init_glfw_callbacks(GLFWwindow* window) {
#ifdef DEBUG
    if (glDebugMessageCallback != NULL && glDebugMessageControl != NULL) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(_message_callback, 0);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    }
#endif
    glfwSetKeyCallback(window, _key_callback);
    glfwSetMouseButtonCallback(window, _mouse_callback);
    glfwSetFramebufferSizeCallback(window, _window_resize_callback);
}

void init_gl_settings(void) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(seeds[0]) * SEED_COUNT, seeds, GL_DYNAMIC_DRAW);

    {
        glEnableVertexAttribArray(ATTRIB_POS);
        glVertexAttribPointer(ATTRIB_POS,
                              2,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(seeds[0]),
                              (void*)0);
        glVertexAttribDivisor(ATTRIB_POS, 1);
    }
    {
        glEnableVertexAttribArray(ATTRIB_COLOR);
        glVertexAttribPointer(ATTRIB_COLOR,
                              4,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(seeds[0]),
                              (void*)(sizeof(float) * 2));
        glVertexAttribDivisor(ATTRIB_COLOR, 1);
    }
    {
        glEnableVertexAttribArray(ATTRIB_RADIUS);
        glVertexAttribIPointer(ATTRIB_RADIUS,
                               1,
                               GL_INT,
                               sizeof(seeds[0]),
                               (void*)(sizeof(float) * 6));
        glVertexAttribDivisor(ATTRIB_RADIUS, 1);
    }
}

void init_shaders(GLuint* program, const char* vert_file_path, const char* frag_file_path) {
    if (!_load_shader_program(vert_file_path, frag_file_path, program)) {
        exit(1);
    }
}

// Private function definitions
// ---------------------
const char* _shader_type_as_cstr(GLuint shader) {
    switch (shader) {
        case GL_VERTEX_SHADER:
            return "GL_VERTEX_SHADER";
        case GL_FRAGMENT_SHADER:
            return "GL_FRAGMENT_SHADER";
        default:
            return "(Unknown)";
    }
}

char* _slurp_file_into_malloced_cstr(const char* file_path) {
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

bool _compile_shader_source(const GLchar* source, GLenum shader_type, GLuint* shader) {
    *shader = glCreateShader(shader_type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);

        fprintf(stderr, "[ERROR]: Could not compile %s\n", _shader_type_as_cstr(shader_type));
        fprintf(stderr, "%.*s\n", message_size, message);
        return false;
    }

    return true;
}

bool _compile_shader_file(const char* file_path, GLenum shader_type, GLuint* shader) {
    char* source = _slurp_file_into_malloced_cstr(file_path);
    if (source == NULL) {
        fprintf(stderr, "[ERROR]: Failed to read file file `%s`: %s\n", file_path, strerror(errno));
        errno = 0;
        return false;
    }
    bool ok = _compile_shader_source(source, shader_type, shader);
    if (!ok) {
        fprintf(stderr, "[ERROR]: Failed to compile `%s` shader file\n", file_path);
    }
    free(source);
    return ok;
}

bool _link_program(GLuint vert_shader, GLuint frag_shader, GLuint* program) {
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

bool _load_shader_program(const char* vertex_file_path, const char* fragment_file_path, GLuint* program) {
    GLuint vert = 0;
    if (!_compile_shader_file(vertex_file_path, GL_VERTEX_SHADER, &vert)) {
        return false;
    }

    GLuint frag = 0;
    if (!_compile_shader_file(fragment_file_path, GL_FRAGMENT_SHADER, &frag)) {
        return false;
    }

    if (!_link_program(vert, frag, program)) {
        return false;
    }

    return true;
}

// Callbacks
void _message_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                      GLsizei length, const GLchar* message, const void* userParam) {
    UNUSED(source);
    UNUSED(id);
    UNUSED(length);
    UNUSED(userParam);

    fprintf(stderr, "GL CALLBACK: %s source = 0x%x, type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            source, type, severity, message);
}

void _window_resize_callback(GLFWwindow* window, int width, int height) {
    UNUSED(window);
    glViewport(0, 0, width, height);
}

void _key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    UNUSED(scancode);
    UNUSED(mods);
    UNUSED(window);

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_SPACE) {
            IS_PAUSE = !IS_PAUSE;
        } else if (key == GLFW_KEY_Q) {
            IS_RUNNING = false;
        }

        if (IS_PAUSE) {
            if (key == GLFW_KEY_LEFT) {
                DELTA_TIME = -MANUAL_TIME_STEP;
            } else if (key == GLFW_KEY_RIGHT) {
                DELTA_TIME = MANUAL_TIME_STEP;
            }
        }
    }

    if (action == GLFW_RELEASE) {
        DELTA_TIME = !(key == GLFW_KEY_LEFT && DELTA_TIME < 0.0f) * DELTA_TIME;
        DELTA_TIME = !(key == GLFW_KEY_RIGHT && DELTA_TIME > 0.0f) * DELTA_TIME;
    }
}

void _mouse_callback(GLFWwindow* window, int button, int action, int mods) {
    UNUSED(mods);
    UNUSED(window);

    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
        IS_DRAG_MODE = true;
    }

    if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT) {
        IS_DRAG_MODE = false;
    }
}