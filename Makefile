# Detect OS
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	SDL_CFLAGS := $(shell sdl2-config --cflags)
	SDL_LDFLAGS := $(shell sdl2-config --libs)
else
	SDL_CFLAGS := -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
	SDL_LDFLAGS := -L/opt/homebrew/lib -lSDL2
endif

CC = gcc
OUT_DIR = out
CC_FLAGS = $(SDL_CFLAGS) -I src/ -D_THREAD_SAFE $(SDL_LDFLAGS)


all: main out_dir
	@echo "Build complete. Executable is located in $(OUT_DIR)/main"

out_dir:
	@mkdir -p $(OUT_DIR)

run: all
	@echo "Running the program..."
	./$(OUT_DIR)/main

main: out_dir src/main.c 
	$(CC) $(CC_FLAGS) -o $(OUT_DIR)/main src/main.c
	chmod +x $(OUT_DIR)/main
	@echo "Compiled main.c to $(OUT_DIR)/main.o"

clean:
	rm -rf $(OUT_DIR)

test: all
	./$(OUT_DIR)/main Tetris.ch8