#include <signal.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <math.h>
#include <stdint.h>
#include <string.h>

#define assert(condition)                                                    \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "Assertion failed [%u]: " #condition, __LINE__); \
            __builtin_debugtrap();                                           \
        }                                                                    \
    } while (0)

#define PI 3.1415926
#define DEGREE_TO_RAD(x) (2 * PI * ((x) / 360.0))

#define SCREEN_TERM_RATIO 1
#define EMPTY_CHAR ' '

typedef struct {
    uint32_t offset_x;
    uint32_t offset_y;
    uint32_t pivot_x;
    uint32_t pivot_y;
    uint32_t width;
    uint32_t height;
    float angle;
} Cube;

typedef struct {
    uint32_t width;
    uint32_t height;
    char *buffer;
} Surface;

static void surface_clear(Surface *surface) {
    memset(surface->buffer, EMPTY_CHAR, surface->width * surface->height);
}

static void surface_set(Surface *surface, int32_t x, int32_t y, char c) {
    if(x >= surface->width || y >= surface->height) return;
    if(y * surface->width + x > surface->width * surface->height) return;

    surface->buffer[y * surface->width + x] = c;
}

static void surface_draw_cube(Surface *surface, Cube *cube) {
    for (uint32_t y = 0; y < cube->height; y++) {
        for (uint32_t x = 0; x < cube->width; x++) {
            float actual_x = x - cube->pivot_x;
            float actual_y = y - cube->pivot_y;
            float pos_x = actual_x * cos(cube->angle) - actual_y * sin(cube->angle) + cube->pivot_x;
            float pos_y = actual_x * sin(cube->angle) + actual_y * cos(cube->angle) + cube->pivot_y;

            surface_set(surface, (int)pos_x + cube->offset_x, (int)pos_y + cube->offset_y, '#');
        }
    }
}

static void cube_rotate(Cube *cube, uint32_t angle) {
    assert(angle < 360);
    cube->angle = DEGREE_TO_RAD(angle);
}

static inline void screen_draw(Surface *surface) {
    for (uint32_t y = 0; y < surface->height * SCREEN_TERM_RATIO; y++) {
        fwrite(surface->buffer + surface->width * y, 1, surface->width, stdout);
        putchar('\n');
    }
}

static uint8_t is_running = 1;
static void restore_terminal(int _) {
    is_running = 0;
}

int main(void) {
    struct winsize window;
    ioctl(0, TIOCGWINSZ, &window);
    signal(SIGINT, restore_terminal);

    Cube cube = {
        .width = 20,
        .height = 20,
        .angle = 0,

        .pivot_x = 0,
        .pivot_y = 0,

        .offset_x = window.ws_row / 2 - 10,
        .offset_y = window.ws_col / 2 - 10,
    };

    Surface surface = {
        .width = window.ws_row,
        .height = window.ws_col,
        .buffer = calloc(window.ws_col * window.ws_row, 1),
    };

    // Clear output buffer
    printf("\x1b[2J");

    uint32_t angle = 0;
    while (is_running) {
        cube_rotate(&cube, angle);
        angle = (angle + 4) % 360;

        surface_clear(&surface);
        surface_draw_cube(&surface, &cube);

        printf("\x1b[H");
        screen_draw(&surface);

        usleep(1000 * 500);
    }

    free(surface.buffer);

    return 0;
}
