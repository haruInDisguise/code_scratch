CC = clang
CFLAGS = -O0 -ggdb

TARGET = ./pv_splice
SRC = pv_splice.c

BENCH_SOURCE= ./yes

all: build

build:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

bench: build
	@eval "$(BENCH_SOURCE) | $(TARGET) > /dev/null"
