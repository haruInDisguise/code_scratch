BUILD_PATH = build
SRC := $(wildcard *.s)
TARGET := $(patsubst %.s,$(BUILD_PATH)/%,$(SRC))
OBJ := $(patsubst %.s,$(BUILD_PATH)/%.o,$(SRC))

all: $(TARGET)

clean:
	rm -rf $(BUILD_PATH)

.PHONY: clean

$(TARGET): %: %.o
	ld $< -o $@

$(OBJ): $(BUILD_PATH)/%.o: %.s
	@mkdir -p $(dir $@)
	yasm -w+all -O0 -F dwarf -g -f elf64 $< -o $@
