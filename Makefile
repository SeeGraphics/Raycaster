# =========================
# Config (project-local)
# =========================
CIMGUI_DIR   ?= third_party/cimgui
ifeq ($(wildcard $(CIMGUI_DIR)/cimgui.h),)
  $(error cimgui.h not found at $(CIMGUI_DIR)/cimgui.h)
endif

# =========================
# Compilers and flags
# =========================
CC   = gcc
CXX  = g++

CFLAGS   = -Wall -Wextra -std=c11   -Iinclude -Ithird_party `sdl2-config --cflags` -g -MMD -MP
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude -Ithird_party `sdl2-config --cflags` -g -MMD -MP

# cimgui includes + OpenGL loader define (GLEW)
IMGUI_INCLUDES   = -I$(CIMGUI_DIR) -I$(CIMGUI_DIR)/imgui -I$(CIMGUI_DIR)/imgui/backends
GL_LOADER_DEFINE = -DIMGUI_IMPL_OPENGL_LOADER_GLEW
CFLAGS   += $(IMGUI_INCLUDES) $(GL_LOADER_DEFINE)
CXXFLAGS += $(IMGUI_INCLUDES) $(GL_LOADER_DEFINE) -DCIMGUI_USE_SDL2 -DCIMGUI_USE_OPENGL3

# Platform OpenGL + rpath
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    OPENGL_LIB = -framework OpenGL
    # build/editor soll die dylib im Repo zur Laufzeit finden
    RPATH_FLAG = -Wl,-rpath,@executable_path/../third_party/cimgui/build
else
    OPENGL_LIB = -lGL -ldl -lpthread
    RPATH_FLAG =
endif

# Linker libs common
LDFLAGS = `sdl2-config --libs` -lSDL2_ttf -lSDL2_image -lSDL2_mixer

# =========================
# Dirs
# =========================
SRC_DIR   = src
BUILD_DIR = build

# =========================
# Game (C)
# =========================
SOURCES = main.c engine.c input.c map.c graphics.c player.c camera.c \
          raycast.c font.c texture.c sprites.c sound.c render.c animation.c \
          weapons.c entities.c enemies.c
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)
DEPS    = $(OBJECTS:.o=.d)
TARGET  = $(BUILD_DIR)/raycast

# =========================
# Editor (cimgui-powered editor UI)
# =========================
EDITOR_C_SOURCES   = editor_main.c
EDITOR_CPP_SOURCES = editor_imgui_backend.cpp
EDITOR_C_OBJS      = $(EDITOR_C_SOURCES:%.c=$(BUILD_DIR)/%.o)
EDITOR_CPP_OBJS    = $(EDITOR_CPP_SOURCES:%.cpp=$(BUILD_DIR)/%.o)

IMGUI_BACKENDS_CPP  = $(CIMGUI_DIR)/imgui/backends/imgui_impl_sdl2.cpp \
                      $(CIMGUI_DIR)/imgui/backends/imgui_impl_opengl3.cpp
IMGUI_BACKENDS_OBJS = $(IMGUI_BACKENDS_CPP:$(CIMGUI_DIR)/imgui/backends/%.cpp=$(BUILD_DIR)/imgui_backend_%.o)

IMGUI_CORE_CPP  = $(CIMGUI_DIR)/imgui/imgui.cpp \
                  $(CIMGUI_DIR)/imgui/imgui_demo.cpp \
                  $(CIMGUI_DIR)/imgui/imgui_draw.cpp \
                  $(CIMGUI_DIR)/imgui/imgui_tables.cpp \
                  $(CIMGUI_DIR)/imgui/imgui_widgets.cpp
IMGUI_CORE_OBJS = $(IMGUI_CORE_CPP:$(CIMGUI_DIR)/imgui/%.cpp=$(BUILD_DIR)/imgui_core_%.o)

CIMGUI_CPP      = $(CIMGUI_DIR)/cimgui.cpp
CIMGUI_OBJ      = $(BUILD_DIR)/cimgui.o

EDITOR_TARGET  = $(BUILD_DIR)/editor

EDITOR_LDFLAGS = $(LDFLAGS) -lGLEW $(OPENGL_LIB)

# =========================
# Targets
# =========================
.PHONY: all run clean editor

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

editor: $(EDITOR_TARGET)

# link editor: editor objs + reuse engine objs (ohne main.o)
$(EDITOR_TARGET): $(EDITOR_C_OBJS) $(EDITOR_CPP_OBJS) $(IMGUI_BACKENDS_OBJS) $(IMGUI_CORE_OBJS) $(CIMGUI_OBJ) $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	@mkdir -p $(dir $@)
	$(CXX) $^ -o $@ $(EDITOR_LDFLAGS)

# =========================
# Compile rules
# =========================
# --- Editor .c als C++ kompilieren ---
$(BUILD_DIR)/editor_%.o: $(SRC_DIR)/editor_%.c
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -DCIMGUI_DEFINE_ENUMS_AND_STRUCTS -c $< -o $@

# generisch: andere C-Sourcen (Game/Engine)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# generisch: C++-Sourcen im src/-Verzeichnis
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ImGui-Backends (C++)
$(BUILD_DIR)/imgui_backend_%.o: $(CIMGUI_DIR)/imgui/backends/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ImGui core sources
$(BUILD_DIR)/imgui_core_%.o: $(CIMGUI_DIR)/imgui/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# cimgui wrapper (C++)
$(BUILD_DIR)/cimgui.o: $(CIMGUI_CPP)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# =========================
# Deps
# =========================
-include $(DEPS)
-include $(EDITOR_C_OBJS:.o=.d)
-include $(EDITOR_CPP_OBJS:.o=.d)
-include $(IMGUI_BACKENDS_OBJS:.o=.d)
-include $(IMGUI_CORE_OBJS:.o=.d)
-include $(CIMGUI_OBJ:.o=.d)

# =========================
# Utils
# =========================
run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
