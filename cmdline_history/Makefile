CC = clang
CFLAGS = -g3 -O0 -Wall -Wconversion -Wno-sign-conversion -fsanitize=address,undefined
LFLAGS = -fsanitize=address,undefined

build:
	$(CC) $(CFLAGS) main.c -o main

gdbserver: build
	gdbserver --multi localhost:4242
	stty sane

gdb:
	gdb \
		-ex 'target extended-remote localhost:4242' \
		-ex 'set remote exec-file ./main' \
		-ex 'start'

clean:
	rm -f main

.PHONY: build clean gdbserver gdb
