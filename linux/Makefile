CMAKE_INSTALL_PREFIX = $(shell pwd)/local
CMAKE_PREFIX_PATH    = $(shell pwd)/local

build_all: SDL SDL_ttf SDL_mixer lodepng kissfft cJSON libmp3lame ezapp_lib picoc ezapp
build_quick: ezapp_lib picoc ezapp

ezapp:
	../bin/build_tools/gen_version
	mkdir -p build/$@
	cd build/$@; cmake ../../../src/$@ -DCMAKE_PREFIX_PATH=$(CMAKE_PREFIX_PATH)
	cd build/$@; cmake --build . # --verbose

kissfft: CMAKE_MORE_OPTS = -DKISSFFT_STATIC=ON -DKISSFFT_TEST=OFF -DKISSFFT_TOOLS=OFF

SDL SDL_ttf SDL_mixer lodepng kissfft cJSON libmp3lame ezapp_lib picoc:
	mkdir -p build/$@
	cd build/$@; cmake  ../../../src/$@ -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX) $(CMAKE_MORE_OPTS)
	cd build/$@; cmake --build . # --verbose
	cd build/$@; cmake --install .

run: build
	cd ../files; ../linux/build/ezapp/ezapp 2>&1 | ezlogfilter

clean:
	rm -f  ../src/ezapp/version.h
	rm -rf build local

.PHONY: SDL SDL_ttf SDL_mixer lodepng kissfft cJSON libmp3lame ezapp_lib picoc
.PHONY: ezapp run all clean

