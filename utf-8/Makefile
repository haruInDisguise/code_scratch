CC = clang
CFLAGS = -gdwarf-5 -O0 -fno-omit-frame-pointer -fsanitize=undefined
LFLAGS = -fsanitize=undefined

main: main.c
	$(CC) $(CFLAGS) $(LFLAGS) main.c -o main
