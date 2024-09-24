#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <linux/uinput.h>

void write_event(int fd, int type, int code, int value) {
    struct input_event event = {
        .code = code,
        .type = type,
        .value = value,
        .time = {0},
    };

    write(fd, &event, sizeof(event));
}

int main(void) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);

    struct uinput_setup usetup = {
        .id = {
            .bustype = BUS_USB,
            .vendor = 0x1234,
            .product = 0x1234,
        },
    };
    strncpy(usetup.name, "test device", UINPUT_MAX_NAME_SIZE);

    write_event(fd, EV_KEY, BTN_LEFT, 1);
    write_event(fd, EV_SYN, SYN_REPORT, 0);
    usleep(10000);
    write_event(fd, EV_KEY, BTN_LEFT, 0);
    write_event(fd, EV_SYN, SYN_REPORT, 0);

    ioctl(fd, UI_DEV_DESTROY);
    close(fd);

    return 0;
}
