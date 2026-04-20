# Linux Makefile (EECS autograder / local build with SDL2)
# - Uses clang++ so build works in environments where g++ is not installed
# - Produces game_engine_linux in this directory
# - Requires SDL2 dev packages: libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
#   (On Fedora: SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel)

CXX := clang++
CC  := clang
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -Wpedantic -Wno-deprecated-declarations
CFLAGS   := -O3

# SDL2: use pkg-config for -I and -L; fallback to explicit -l when pkg-config --libs is empty (e.g. on some autograders)
SDL2_CFLAGS := $(shell pkg-config --cflags SDL2 SDL2_image SDL2_mixer SDL2_ttf 2>/dev/null)
SDL2_LIBS   := $(shell pkg-config --libs SDL2 SDL2_image SDL2_mixer SDL2_ttf 2>/dev/null)
ifeq ($(SDL2_LIBS),)
SDL2_LIBS := -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lSDL2
endif

INCLUDES := -ISDL2 -Iwindows_sdl_include -I. -Ithird_party -Ithird_party/lua -Irapidjson-1.1.0/include \
           -Ibox2d-2.4.1/include -Ibox2d-2.4.1/src \
           $(SDL2_CFLAGS)
LDFLAGS  := $(SDL2_LIBS)

CPP_SOURCES := $(wildcard *.cpp)
LUA_SOURCES := $(wildcard third_party/lua/*.c)
LUA_OBJECTS := $(LUA_SOURCES:.c=.o)

BOX2D_SOURCES := $(wildcard box2d-2.4.1/src/collision/*.cpp) \
                 $(wildcard box2d-2.4.1/src/common/*.cpp) \
                 $(wildcard box2d-2.4.1/src/dynamics/*.cpp) \
                 $(wildcard box2d-2.4.1/src/rope/*.cpp)
BOX2D_OBJECTS := $(BOX2D_SOURCES:.cpp=.o)

TARGET := game_engine_linux

.PHONY: all clean

all: $(TARGET)

third_party/lua/%.o: third_party/lua/%.c
	$(CC) $(CFLAGS) -c $< -o $@

box2d-2.4.1/src/%.o: box2d-2.4.1/src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(CPP_SOURCES) $(LUA_OBJECTS) $(BOX2D_OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CPP_SOURCES) $(LUA_OBJECTS) $(BOX2D_OBJECTS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(LUA_OBJECTS) $(BOX2D_OBJECTS)
