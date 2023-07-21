CC = clang
CFLAGS = -g3 -O0 -Wall -Wconversion -Wno-sign-conversion -fsanitize=address,undefined
LFLAGS = -fsanitize=address,undefined

build:
	$(CC) $(CFLAGS) main.c -o main

gdbserver: build
	gdbserver localhost:4242 ./main
	stty sane

gdb:
	gdb -ex 'target remote localhost:4242' -ex 'tbreak cmdline_update' -ex 'continue'

clean:
	rm -f main

.PHONY: build clean gdbserver gdb
