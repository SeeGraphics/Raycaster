# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude `sdl2-config --cflags` -g -fsanitize=address
LDFLAGS = `sdl2-config --libs` -lSDL2_ttf -lSDL2_image -lSDL2_mixer -fsanitize=address

# Directories
SRC_DIR = src
BUILD_DIR = build

# Files
SOURCES = main.c engine.c input.c map.c graphics.c player.c camera.c \
           raycast.c font.c texture.c sprites.c sound.c render.c animation.c
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)
DEPS    = $(OBJECTS:.o=.d)
TARGET  = $(BUILD_DIR)/raycast

# Default
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# include header dependencies
-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean

