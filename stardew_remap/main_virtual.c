#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <libevdev/libevdev-uinput.h>

#define KEY_PRESSED  1
#define KEY_REPEAT   2
#define KEY_RELEASED 0

#define TIMEOUT_KEYPRESS 15
#define TIMEOUT_DELAY    150

enum {
    STATE_WAIT,
    STATE_START,
    STATE_MOUSE_PRESSED,
    STATE_DELAY,
    STATE_KEY_PRESSED,
};

void write_event(struct libevdev_uinput *device, unsigned int type, unsigned int code, int value) {
    int result = libevdev_uinput_write_event(device, type, code, value);
    assert(result == 0);
    result = libevdev_uinput_write_event(device, EV_SYN, SYN_REPORT, 0);
    assert(result == 0);
}

uint64_t get_elapsed_ms(struct timespec *now, struct timespec *then) {
    int64_t now_ns = now->tv_sec * 1000000000 + now->tv_nsec;
    int64_t then_ns = then->tv_sec * 1000000000 + then->tv_nsec;
    assert(now_ns >= then_ns);

    return (now_ns - then_ns) / 1000000;
}

int main(void) {
    // TODO: Error handeling...
    // Create hardware keyboard
    int keyboard_fd = open("/dev/input/event6", O_RDONLY, O_NONBLOCK);
    assert(keyboard_fd > 0);

    struct libevdev *hardware_keyboard;
    int err = libevdev_new_from_fd(keyboard_fd, &hardware_keyboard);
    assert(err == 0);

    libevdev_set_name(hardware_keyboard, "stardew_keyboard_or_something");

    // TODO: Figure out the minimal "permissions" necessary to simulate the required keycodes
    int virtual_keyboard_fd = open("/dev/uinput", O_RDWR);
    assert(virtual_keyboard_fd > 0);

    // Create virtual keyboard
    struct libevdev_uinput *virtual_keyboard;
    err = libevdev_uinput_create_from_device(
        hardware_keyboard, virtual_keyboard_fd, &virtual_keyboard
    );
    assert(err == 0);

    // Create the hardware mouse
    // TODO: Figure out the minimal "permissions" necessary to simulate the required keycodes
    int mouse_fd = open("/dev/input/event3", O_RDONLY);
    assert(mouse_fd > 0);

    struct libevdev *hardware_mouse;
    err = libevdev_new_from_fd(mouse_fd, &hardware_mouse);
    assert(err == 0);

    int virtual_mouse_fd = open("/dev/uinput", O_RDWR);
    struct libevdev_uinput *virtual_mouse;
    err = libevdev_uinput_create_from_device(hardware_mouse, virtual_mouse_fd, &virtual_mouse);
    assert(err == 0);

    close(mouse_fd);

    bool is_running = true;
    bool is_key_pressed = false;
    bool is_key_tapped = false;

    int result = 0;
    const uint32_t timeout_pressed = 120;
    const uint32_t timeout_delay = 30;

    struct timespec now, then;
    clock_gettime(CLOCK_MONOTONIC, &then);
    int state = STATE_WAIT;

    // EAGAIN: All current events have been read
    while (result >= 0 || result == -EAGAIN) {
        struct input_event event = { 0 };
        result = -EAGAIN;

        // NOTE: Keyboard input is so slow, that it is (probably) unlikely that packets will be dropped
        //       I just assume that traffic is always in sync
        if (libevdev_has_event_pending(hardware_keyboard)) {
            result = libevdev_next_event(hardware_keyboard, LIBEVDEV_READ_FLAG_NORMAL, &event);
        }

        if (result == LIBEVDEV_READ_STATUS_SUCCESS && event.type == EV_KEY && event.code == KEY_SPACE) {
            switch (event.value) {
            case KEY_PRESSED:
                is_key_tapped = true;

                break;
            case KEY_RELEASED:

                if (is_key_tapped) {
                    printf("sequence: tapped\n");
                    state = STATE_START;
                    continue;
                }

                printf("sequence: interrupt\n");

                is_key_pressed = false;
                state = STATE_WAIT;

                /* Restore a sane state */
                write_event(virtual_mouse, EV_KEY, BTN_LEFT, KEY_RELEASED);
                write_event(virtual_keyboard, EV_KEY, KEY_SPACE, KEY_RELEASED);

                break;
            case KEY_REPEAT:
                if (state <= STATE_WAIT) {
                    is_key_tapped = false;
                    state = STATE_START;
                }

                break;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &now);
        uint64_t elapsed_ms = get_elapsed_ms(&now, &then);

        switch (state) {
        case STATE_WAIT:
            break;
        case STATE_START:
            printf("sequence: state: start mouse\n");
            printf("sequence: send key down\n");

            write_event(virtual_mouse, EV_KEY, BTN_LEFT, KEY_PRESSED);
            clock_gettime(CLOCK_MONOTONIC, &then);

            state = STATE_MOUSE_PRESSED;
            break;
        case STATE_MOUSE_PRESSED:

            /* Keep the keys pressed for '' ms */
            if (elapsed_ms >= TIMEOUT_KEYPRESS) {
                printf("sequence: state: end mouse\n");
                printf("sequence: send key up\n");

                write_event(virtual_mouse, EV_KEY, BTN_LEFT, KEY_RELEASED);
                clock_gettime(CLOCK_MONOTONIC, &then);

                state = STATE_DELAY;
            }

            break;
        case STATE_DELAY:

            if (elapsed_ms >= TIMEOUT_DELAY) {
                printf("sequence: state: start key\n");
                printf("sequence: send key down\n");

                write_event(virtual_keyboard, EV_KEY, KEY_RIGHTSHIFT, KEY_PRESSED);
                write_event(virtual_keyboard, EV_KEY, KEY_DELETE, KEY_PRESSED);
                write_event(virtual_keyboard, EV_KEY, KEY_R, KEY_PRESSED);
                clock_gettime(CLOCK_MONOTONIC, &then);

                state = STATE_KEY_PRESSED;
            }

            break;
        case STATE_KEY_PRESSED:

            if (elapsed_ms >= TIMEOUT_KEYPRESS) {
                printf("sequence: state: end key\n");
                printf("sequence: send key up\n");
                printf("sequence: end\n");

                write_event(virtual_keyboard, EV_KEY, KEY_R, KEY_RELEASED);
                write_event(virtual_keyboard, EV_KEY, KEY_DELETE, KEY_RELEASED);
                write_event(virtual_keyboard, EV_KEY, KEY_RIGHTSHIFT, KEY_RELEASED);
                clock_gettime(CLOCK_MONOTONIC, &then);

                state = STATE_WAIT;
            }

            break;
        }
    }

    libevdev_uinput_destroy(virtual_keyboard);
    close(virtual_keyboard_fd);

    return 0;
}
