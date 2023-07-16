CC = clang++

CFLAGS = -O0 -g3 -fno-omit-frame-pointer -Wall -Wextra -Wconversion -Wno-sign-conversion -Wno-unused-parameter
CFLAGS += -fsanitize=address
LFLAGS = -fsanitize=address
# My distributions glfw package's 'glfwInit()' call takes quite a while to complete (see: https://github.com/glfw/glfw/issues/1471).
LFLAGS += -lglbinding -Wl,-rpath,$(PWD)/lib/glfw/build/src -L$(PWD)/lib/glfw/build/src -lglfw

SRC = main.cpp
OBJ = $(patsubst %.cpp,%.o,$(SRC))

BUILD_PATH = .
TARGET = $(BUILD_PATH)/main

build:
	$(CC) $(CFLAGS) $(LFLAGS) $(SRC) -o $(TARGET)

run: build
	eval $(TARGET)

