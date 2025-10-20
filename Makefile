CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_ttf -lSDL2_image
SRC_DIR = src
BUILD_DIR = build

SOURCES = main.c map.c graphics.c player.c camera.c raycast.c font.c texture.c
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/raycast

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run

