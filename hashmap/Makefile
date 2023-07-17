CC = clang
CFLAGS = -O0 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined

build: hashmap.h
	$(CC) $(CFLAGS) hashmap.c main.c -o main

