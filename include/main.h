#ifndef _MAIN_H
#define _MAIN_H

#include <stdbool.h>

#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
#include "glextloader.h"

#include "helpers.h"

// Macros
#define UNREACHABLE(message)                                                      \
    do {                                                                          \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message); \
        exit(1);                                                                  \
    } while (0)

// Constants
// ---------------------
// Window properties
#define DEFAULT_SCREEN_WIDTH 1920
#define DEFAULT_SCREEN_HEIGHT 1080
#define MANUAL_TIME_STEP 0.05

// Seed properties
#define SEED_COUNT 20
#define SEED_MARK_RADIUS 7
#define BUBBLE_MAX_RADIUS 30
#define BUBBLE_MIN_RADIUS 150
#define SEED_MARK_COLOR ((vec4){0.0f, 0.0f, 0.0f, 1.0f})

// Physics properties
#define GRAVITY ((vec2){0.0f, -9.8f})
#define SUB_STEPS 8

// Shader paths
#define VERTEX_FILE_PATH "shaders/quad.vert"
#define VORONOI_FRAGMENT_FILE_PATH "shaders/voronoi.frag"
#define BALLS_FRAGMENT_FILE_PATH "shaders/balls.frag"
#define BUBBLES_FRAGMENT_FILE_PATH "shaders/bubbles.frag"

typedef enum {
    ATTRIB_POS = 0,
    ATTRIB_COLOR,
    ATTRIB_RADIUS,
    COUNT_ATTRIBS,
} Attrib;

typedef enum {
    MODE_VORONOI,
    MODE_BALLS,
    MODE_BUBBLES,
} Mode;

typedef enum {
    RESOLUTION_UNIFORM = 0,
    SEED_MARK_COLOR_UNIFORM,
    SEED_MARK_RADIUS_UNIFORM,
    COUNT_UNIFORMS
} Uniform;

extern const char* uniform_names[COUNT_UNIFORMS];

extern vec2 seed_positions[SEED_COUNT];
extern vec2 seed_velocities[SEED_COUNT];
extern vec4 seed_colors[SEED_COUNT];
extern GLint seed_mark_radii[SEED_COUNT];

extern bool pause;
extern Mode mode;
extern bool drag_mode;
extern double global_delta_time;

extern GLint uniforms[COUNT_UNIFORMS];
extern GLuint vbos[COUNT_ATTRIBS];

extern void (*generate_seeds)(void);
extern void (*render_frame)(GLFWwindow*, double, int, int);

// Function declarations
// ---------------------
void render_loop(GLFWwindow* window);
void init_mode(int argc, char** argv);

// OpenGL functions 
const char* shader_type_as_cstr(GLuint shader);
char* slurp_file_into_malloced_cstr(const char* file_path);
bool compile_shader_source(const GLchar* source, GLenum shader_type, GLuint* shader);
bool compile_shader_file(const char* file_path, GLenum shader_type, GLuint* shader);
bool link_program(GLuint vert_shader, GLuint frag_shader, GLuint* program);
bool load_shader_program(const char* vertex_file_path, const char* fragment_file_path, GLuint* program);

void init_glfw_settings();
GLFWwindow* init_glfw_window();
void init_glfw_callbacks(GLFWwindow* window);
void init_gl_settings();
void init_gl_uniforms(GLuint program);
void init_shaders(GLuint* program);
void update_gl_uniforms(int width, int height);

void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
void window_resize_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, int button, int action, int mods);

// Sim functions
void generate_bubbles_seeds();
void generate_voronoi_seeds();

void render_voronoi_frame(GLFWwindow* window, double delta_time, int width, int height);
void render_bubbles_frame(GLFWwindow* window, double delta_time, int width, int height);

#endif  // MAIN_H