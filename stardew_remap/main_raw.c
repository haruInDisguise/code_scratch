#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libevdev/libevdev-uinput.h>

void write_event(int fd, unsigned int type, unsigned int code, int value) {
    struct input_event ev = {
        .code = code,
        .type = type,
        .value = value,
    };

    int status = write(fd, &ev, sizeof(ev));
    assert(status != -1);
}

// code: 0 - released
// code: 1 - pressed
// code: 2 - repeat
void send_keydown(int fd, int keycode) {
    write_event(fd, MSC_SCAN, keycode, 1);
    write_event(fd, EV_KEY, keycode, 1);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
}

void send_keyup(int fd, int keycode) {
    write_event(fd, EV_KEY, keycode, 0);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
}

void send_scan(int fd, int value) {
    // TODO: Figure out a way to compute/obtain the scan code
    // FIXME: Currently just sending the scan code for KEY_ESC
    write_event(fd, EV_MSC, MSC_SCAN, 0x70029);
}

void send_keycodes(int fd) {
#if 0
    send_keydown(device, KEY_RIGHTSHIFT);
    send_keydown(device, KEY_DELETE);
    send_keydown(device, KEY_R);

    usleep(10 * 1000);

    send_keyup(device, KEY_RIGHTSHIFT);
    send_keyup(device, KEY_DELETE);
    send_keyup(device, KEY_LEFTCTRL);
#endif

    /*send_scan(fd, KEY_ESC);*/
    send_keydown(fd, KEY_E);
    usleep(1000);
    /*send_scan(fd, KEY_ESC);*/
    send_keyup(fd, KEY_E);
}

void print_device_summary(struct libevdev *device) {
    printf("Device name: %s\n ", libevdev_get_name(device));
    printf(
        "Device ID: bus '%#x' vendor '%x' product '%x'\n", libevdev_get_id_bustype(device),
        libevdev_get_id_vendor(device), libevdev_get_id_product(device)
    );
}

int main(void) {
    // TODO: Error handeling...
    int mouse_fd = open("/dev/input/event3", O_RDONLY);
    assert(mouse_fd != -1);

    struct libevdev *mouse;
    int err = libevdev_new_from_fd(mouse_fd, &mouse);
    assert(err == 0);

    int keyboard_fd = open("/dev/input/event6", O_RDWR);
    assert(keyboard_fd != -1);

    print_device_summary(mouse);

    if (!libevdev_has_event_code(mouse, EV_KEY, BTN_LEFT)) {
        printf("This device does not look like a mouse\n");
        exit(1);
    }

    int is_running = 1;
    int result = 0;
    // EAGAIN: All current events have been read
    while(result >= 0 || result == -EAGAIN) {
        struct input_event ev;
        result = libevdev_next_event(mouse, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if(result == LIBEVDEV_READ_STATUS_SUCCESS && ev.type == EV_KEY && ev.code == BTN_LEFT && ev.value == 0) {
            printf("sending keycodes\n");
            usleep(150 * 1000);
            send_keycodes(keyboard_fd);
        }
    }

    libevdev_free(mouse);
    close(mouse_fd);
    return 0;
}
