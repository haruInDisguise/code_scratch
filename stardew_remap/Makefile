CC = clang
# CFLAGS = -O0 -fno-omit-frame-pointer -fsanitize=address -ggdb
CFLAGS = -O3

main: main.c
	$(CC) $(CFLAGS) -levdev -I/usr/include/libevdev-1.0 -o main main.c -fsanitize=address

main_uinput: main_uinput.c
	$(CC) $(CFLAGS) -levdev -I/usr/include/libevdev-1.0 -o main_uinput main_uinput.c

clean:
	rm main

.PHONY: clean
