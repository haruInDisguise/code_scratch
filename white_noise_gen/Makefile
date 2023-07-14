CC = clang
CFLAGS = -g3 -O0 -fno-omit-frame-pointer -Wall -Wextra
LFLAGS = -lSDL2 -D_REENTRANT -lm

build: main.c
	$(CC) $(CFLAGS) $(LFLAGS) main.c -o main

.PHONY: build
