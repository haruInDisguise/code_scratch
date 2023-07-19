CC = clang
CFLAGS = -O0 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined
#CFLAGS = -O3

build: hashmap.h
	$(CC) $(CFLAGS) hashmap.c main.c -o main

preprocess: scripts/preprocess_testcases.c
	$(CC) -O3 $< -o $@

.PHONY: build preprocess

