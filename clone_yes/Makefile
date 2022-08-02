TARGET=yes_vmsplice

all:
	clang -pipe -march=native -mtune=native -Os yes_vmsplice.c -o $(TARGET)

bench: all
	@eval "./$(TARGET) | pv >/dev/null"

clean:
	rm -rf $(TARGET)

.PHONY: all bench clean
