CMAKE_INSTALL_PREFIX = $(shell pwd)/local
CMAKE_PREFIX_PATH    = $(shell pwd)/local

all: SDL SDL_ttf SDL_mixer lodepng cJSON mp3lame ezapp_lib picoc ezapp

ezapp:
	../bin/build_tools/gen_version
	mkdir -p build/$@
	cd build/$@; cmake ../../../src/$@ -DCMAKE_PREFIX_PATH=$(CMAKE_PREFIX_PATH)
	cd build/$@; cmake --build . # --verbose

SDL SDL_ttf SDL_mixer lodepng cJSON mp3lame ezapp_lib picoc:
	mkdir -p build/$@
	cd build/$@; cmake  ../../../src/$@ -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)
	cd build/$@; cmake --build . # --verbose
	cd build/$@; cmake --install .

build: ezapp_lib picoc ezapp

run: build
	cd ../files; ../linux/build/ezapp/ezapp

clean:
	rm -rf build local

.PHONY: SDL SDL_ttf SDL_mixer lodepng cJSON mp3lame ezapp_lib picoc
.PHONY: ezapp run all clean

