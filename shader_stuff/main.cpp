#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_CHECK(condition)                                                  \
    do {                                                                       \
        if (!(condition)) {                                                    \
            static const char *message = NULL;                                 \
            glfwGetError(&message);                                            \
            fprintf(stderr, "GLFW: error: %s\n", message);                     \
            terminate(1);                                                      \
        }                                                                      \
    } while (0)

static inline void terminate(int status) {
    glfwTerminate();

    exit(status);
}

static inline void shader_validate(gl::GLuint shader_id) {
    gl::GLint success;
    char log_buffer[512] = { 0 };
    gl::glGetShaderiv(shader_id, gl::GL_COMPILE_STATUS, &success);

    if (!success) {
        gl::glGetShaderInfoLog(shader_id, 512, NULL, log_buffer);
        fprintf(stderr, "Error: Faild to compile shader: %s", log_buffer);
        terminate(1);
    }
}

static char *load_file(const char *path) {
    FILE *file = fopen(path, "r");

    if (file == NULL) {
        perror("Error: load_file()");
        terminate(errno);
    }

    fseek(file, 0, SEEK_END);

    long size = ftell(file);
    rewind(file);

    void *buffer = malloc(size + 1);
    if (buffer == NULL) {
        perror("Error: load_file()");
        terminate(errno);
    }

    if (fread(buffer, 1, size, file) < size) {
        perror("Error: load_file()");
        terminate(errno);
    }

    fclose(file);

    return (char*)buffer;
}

// Callbacks for GLFW
void callback_glfw_error(int error, const char *message) {
    fprintf(stderr, "GLFW Error: %s\n", message);
    terminate(1);
}

void callback_glfw_framebuffer_size(GLFWwindow *window, int width, int height) {
    printf("Size: %d %d\n", width, height);
    gl::glViewport(0, 0, width, height);
}

void callback_glfw_input_key(GLFWwindow *window, int key, int scancode,
                             int action, int mods) {
    switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        if (action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
}

#define VERTEX_SHADER_PATH "assets/vertex_shader.glsl"
#define FRAGMENT_SHADER_PATH "assets/fragment_shader.glsl"

#define WIDTH 500u
#define HEIGHT 300u

void cleanup(void) {
}

int main(void) {
    using namespace gl;

    glfwSetErrorCallback(callback_glfw_error);

    GLFW_CHECK(glfwInit() == GLFW_TRUE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(
        WIDTH, HEIGHT, "Mandelbrot in GLSL - Press ESC/Q to quit", NULL, NULL);
    GLFW_CHECK(window != NULL);

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, callback_glfw_framebuffer_size);
    glfwSetKeyCallback(window, callback_glfw_input_key);

    glbinding::initialize(glfwGetProcAddress);

    // Load vertex shader
    char *vertex_shader_source = load_file(VERTEX_SHADER_PATH);
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    shader_validate(vertex_shader);

    // load fragment shader
    char *fragment_shader_source = load_file(FRAGMENT_SHADER_PATH);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    shader_validate(fragment_shader);

    // ... and link them together
    // TODO: Validate program integrity
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    struct {
        GLfloat x;
        GLfloat y;
        GLfloat z;
    } vertices[] = {
        { -0.5f, -0.5f, 0.0f, },
        { -0.5f,  0.5f, 0.0f, },
        {  0.5f,  0.5f, 0.0f, },
        {  0.5f, -0.5f, 0.0f, },
    };

    GLuint indices[] = {
        0, 1, 2,
        0, 3, 2
    };

    // Creat VBO, EAO and binde them to a VAO
    // A VertexArrayObject keeps track of:
    // - Calls to glBindBuffer(...)
    GLuint vertex_buffer_obj, vertex_buffer_array, element_buffer_obj;
    glGenVertexArrays(1, &vertex_buffer_array);
    glGenBuffers(1, &vertex_buffer_obj);
    glGenBuffers(1, &element_buffer_obj);

    glBindVertexArray(vertex_buffer_array);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_obj);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_obj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Tell OpenGL about our vertex layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void *)0);
    glEnableVertexAttribArray(0);

    glUseProgram(shader_program);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteProgram(shader_program);

    glDeleteVertexArrays(1, &vertex_buffer_array);
    glDeleteBuffers(1, &vertex_buffer_obj);

    free(vertex_shader_source);
    free(fragment_shader_source);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
