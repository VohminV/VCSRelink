ROM_BIN := build/game.bin
GAME := build/relinked_game.cpp

CXX ?= g++
CXXFLAGS ?= -O2 -std=gnu++17 -Wall -Wextra -Isrc
SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS := $(shell sdl2-config --libs)

all: vcsrelink

build:
	@mkdir -p build

build/relinker: src/relinker.cpp | build
	$(CXX) -O2 -std=gnu++17 -Wall -Wextra -o $@ src/relinker.cpp

$(ROM_BIN): | build
	cp "$$(ls Rom/*.bin | head -n 1)" $@

$(GAME): build/relinker $(ROM_BIN) | build
	./build/relinker $(ROM_BIN) $(GAME)

build/relinked_game.o: $(GAME) src/runtime.hpp
	$(CXX) -O1 -std=gnu++17 -Isrc $(SDL_CFLAGS) -c -o $@ $(GAME)

build/runtime.o: src/runtime.cpp src/runtime.hpp src/font5x7.hpp
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c -o $@ src/runtime.cpp

build/main.o: src/main.cpp src/runtime.hpp
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -c -o $@ src/main.cpp

vcsrelink: build/main.o build/runtime.o build/relinked_game.o
	$(CXX) -o $@ $^ $(SDL_LIBS)

run: vcsrelink
	./vcsrelink

gen: $(GAME)

clean:
	rm -rf build vcsrelink

.PHONY: all run gen clean
