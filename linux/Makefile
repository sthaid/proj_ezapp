CMAKE_INSTALL_PREFIX = $(PWD)/local
CMAKE_PREFIX_PATH    = $(PWD)/local

ezapp:
	../bin/build_tools/gen_version
	mkdir -p build/$@
	cd build/$@; cmake ../../../src/$@ -DCMAKE_PREFIX_PATH=$(CMAKE_PREFIX_PATH)
	cd build/$@; cmake --build .

SDL SDL_ttf SDL_mixer picoc cJSON lame:
	mkdir -p build/$@
	cd build/$@; cmake  ../../../src/$@ -DCMAKE_INSTALL_PREFIX=$(CMAKE_INSTALL_PREFIX)
	cd build/$@; cmake --build . --verbose
	cd build/$@; cmake --install .

all: SDL SDL_ttf SDL_mixer picoc cJSON lame ezapp

mini: picoc cJSON lame ezapp

run: ezapp
	cd ../files; LD_LIBRARY_PATH=../linux/local/lib ../linux/build/ezapp/ezapp

clean:
	rm -rf build local

.PHONY: SDL SDL_ttf SDL_mixer picoc cJSON lame
.PHONY: ezapp run all mini clean

